# SPDX-FileCopyrightText: Copyright 2026 Process Mission
# SPDX-License-Identifier: MIT

"""Build complete Chinese articles while retaining the upstream reference targets."""

import hashlib
import json
import os
import re
from pathlib import Path

from docutils import nodes
from sphinx.errors import ExtensionError
from sphinx.search import SearchLanguage


class ChineseSearch(SearchLanguage):
    """Use matching CJK bigrams in the index and browser, retaining identifiers."""

    lang = 'zh'
    language_name = 'Chinese'
    js_stemmer_rawcode = ''
    js_stemmer_code = (
        'var Stemmer = function() { this.stemWord = function(w) { return w.toLowerCase(); }; };'
    )
    js_splitter_code = r"""
    function splitQuery(query) {
        return (query.match(/[\u3400-\u9fff]+|[a-zA-Z0-9_]+/g) || []).flatMap(word => {
            if (!/^[\u3400-\u9fff]/.test(word) || word.length === 1) return [word];
            return Array.from({length: word.length - 1}, (_, i) => word.slice(i, i + 2));
        });
    }
    """

    def split(self, text):
        words = []
        for word in re.findall(r'[\u3400-\u9fff]+|[a-zA-Z0-9_]+', text):
            if '\u3400' <= word[0] <= '\u9fff':
                words.extend(word)
                words.extend(word[index:index + 2] for index in range(len(word) - 1))
            else:
                words.append(word)
        return words

    def stem(self, word):
        return word.lower()


def translated(app):
    return app.config.language == 'zh_CN'


def section_depth(section):
    depth = 0
    parent = section.parent
    while parent is not None:
        depth += isinstance(parent, nodes.section)
        parent = parent.parent
    return depth


def configure(app, config):
    root = Path(app.confdir)
    if config.language != 'zh_CN':
        return
    from sphinx.search import languages
    languages['zh'] = ChineseSearch
    languages['zh_CN'] = ChineseSearch
    config.locale_dirs = []
    definitions = root / 'translations/zh_CN/defs.rst.inc'
    if definitions.is_file():
        config.rst_epilog = config.rst_epilog.replace(
            (root / 'defs.rst.inc').read_text(encoding='utf-8'),
            definitions.read_text(encoding='utf-8'),
        )
    config.project = 'QEMU 中文文档'
    config.html_title = config.project
    config.html_baseurl = os.environ.get('QEMU_DOCS_SITE_URL', '')
    config.html_last_updated_fmt = '%Y年%m月%d日'
    config.html_copy_source = False
    config.html_show_sourcelink = False
    config.templates_path = [str(root / 'translations/zh_CN/_templates'), *config.templates_path]
    config.html_static_path = [*config.html_static_path, str(root / 'translations/zh_CN/_static')]
    config.html_context = dict(config.html_context)
    config.html_context['upstream_docs_url'] = 'https://www.qemu.org/docs/master/'
    app.add_js_file('search-ui.js', priority=900)
    app.add_js_file('cjk-literals.js', priority=900)
    app.add_css_file('cjk-literals.css')


def load_references(app):
    if translated(app):
        path = Path(app.config.qemu_reference_inventory)
        if not path.is_file():
            raise ExtensionError('Build the English reference inventory before Chinese HTML.')
        app._qemu_references = json.loads(path.read_text(encoding='utf-8'))
        prose = Path(app.confdir) / 'translations/zh_CN/_meta/literal-prose.json'
        app._qemu_literal_prose = json.loads(prose.read_text(encoding='utf-8')) if prose.is_file() else {}


def read_source(app, docname, source):
    if not translated(app):
        return
    path = Path(app.confdir) / 'translations/zh_CN' / (docname + '.rst')
    app.env.note_dependency(str(path))
    app.env.note_dependency(str(Path(app.confdir) / 'translations/zh_CN/_meta/literal-prose.json'))
    if path.is_file():
        source[0] = path.read_text(encoding='utf-8')
    elif app.config.qemu_require_translations:
        raise ExtensionError(f'Missing complete Chinese article: {path}')


def read_include(app, relative_path, parent_docname, content):
    if not translated(app):
        return
    relative = Path(relative_path)
    if relative.is_absolute():
        try:
            relative = relative.relative_to(app.srcdir)
        except ValueError:
            return
    path = Path(app.confdir) / 'translations/zh_CN' / relative
    if path.is_file():
        content[0] = path.read_text(encoding='utf-8')
        app.env.note_dependency(str(path))


def restore_targets(app, doctree):
    if not translated(app):
        return
    name = app.env.docname
    original = app._qemu_references.get(name)
    if original is None:
        return
    sections = list(doctree.findall(nodes.section))
    literal_nodes = list(doctree.findall(nodes.literal_block))
    literals = [node.astext() for node in literal_nodes]
    approved = app._qemu_literal_prose.get(name, {})
    differences = []
    for index, (left, right) in enumerate(zip(original['literals'], literals)):
        if left == right:
            continue
        entry = approved.get(str(index), {})
        if not (
            entry.get('reason')
            and entry.get('source_sha256') == hashlib.sha256(left.encode()).hexdigest()
            and entry.get('translation_sha256') == hashlib.sha256(right.encode()).hexdigest()
        ):
            differences.append(index)
        else:
            literal_nodes[index]['classes'].append('translated-prose')
    if len(original['literals']) != len(literals) or differences:
        raise ExtensionError(
            f'{name}: code blocks or command output differ from the English source; '
            f'counts {len(original["literals"])}/{len(literals)}, blocks {differences}'
        )
    if original.get('images') != [node['uri'] for node in doctree.findall(nodes.image)]:
        raise ExtensionError(f'{name}: image references differ from the English source')
    if len(sections) != len(original['sections']):
        raise ExtensionError(
            f'{name}: section count changed: English {len(original["sections"])}, '
            f'Chinese {len(sections)}; preserve the original heading hierarchy.'
        )
    if [section_depth(section) for section in sections] != [
        section['depth'] for section in original['sections']
    ]:
        raise ExtensionError(f'{name}: heading hierarchy differs from the English source')
    reserved = {identifier for old in original['sections'] for identifier in old['ids']}
    remapped = {}
    for old, section in zip(original['sections'], sections):
        for identifier in section['ids']:
            if old['ids']:
                remapped[identifier] = old['ids'][0]
            doctree.ids.pop(identifier, None)
    for old, section in zip(original['sections'], sections):
        current = [identifier for identifier in section['ids'] if identifier not in reserved]
        section['ids'] = list(dict.fromkeys([*old['ids'], *current]))
        for identifier in section['ids']:
            owner = doctree.ids.get(identifier)
            if owner is not None and owner is not section:
                replacement = 'zh-' + identifier
                while replacement in doctree.ids:
                    replacement = 'zh-' + replacement
                owner['ids'] = [replacement if item == identifier else item for item in owner['ids']]
                doctree.ids[replacement] = owner
                remapped[identifier] = replacement
            doctree.ids[identifier] = section
    for reference in doctree.findall(nodes.Element):
        if reference.get('refid') in remapped:
            reference['refid'] = remapped[reference['refid']]
    domain = app.env.get_domain('std')
    for key, (docname, identifier, title) in list(domain.labels.items()):
        if docname == name and identifier in remapped:
            domain.labels[key] = (docname, remapped[identifier], title)
    for key, (docname, identifier) in list(domain.anonlabels.items()):
        if docname == name and identifier in remapped:
            domain.anonlabels[key] = (docname, remapped[identifier])
    for label, value in original['labels'].items():
        docname, identifier, title = value
        if not identifier:
            domain.labels[label] = (docname, identifier, title)
            domain.anonlabels[label] = (docname, identifier)
            continue
        target = doctree.ids.get(identifier)
        if target is None:
            raise ExtensionError(f'{name}: original reference target disappeared: {identifier}')
        if isinstance(target, nodes.section):
            title = target.next_node(nodes.title).astext()
        domain.labels[label] = (docname, identifier, title)
        domain.anonlabels[label] = (docname, identifier)


def export_references(app, env):
    if translated(app):
        return
    domain = env.get_domain('std')
    result = {}
    for name in sorted(env.found_docs):
        tree = env.get_doctree(name)
        result[name] = {
            'sections': [
                {'ids': section['ids'], 'depth': section_depth(section)}
                for section in tree.findall(nodes.section)
            ],
            'literals': [node.astext() for node in tree.findall(nodes.literal_block)],
            'images': [node['uri'] for node in tree.findall(nodes.image)],
            'labels': {key: value for key, value in domain.labels.items() if value[0] == name},
        }
    path = Path(app.config.qemu_reference_inventory)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def page_context(app, pagename, templatename, context, doctree):
    if translated(app):
        overrides = Path(app.confdir) / 'translations/zh_CN/_meta/english-links.json'
        english_links = json.loads(overrides.read_text(encoding='utf-8')) if overrides.is_file() else {}
        context['english_url'] = english_links.get(
            pagename, app.config.html_context['upstream_docs_url'] + pagename + '.html'
        )
        context['has_chinese_source'] = (
            Path(app.confdir) / 'translations/zh_CN' / (pagename + '.rst')
        ).is_file()


def setup(app):
    app.add_config_value('qemu_reference_inventory', str(Path(app.confdir) / '_build/zh/references.json'), '')
    app.add_config_value('qemu_require_translations', True, 'env')
    app.connect('config-inited', configure)
    app.connect('builder-inited', load_references)
    app.connect('source-read', read_source, priority=100)
    app.connect('include-read', read_include, priority=100)
    app.connect('doctree-read', restore_targets, priority=900)
    app.connect('env-updated', export_references)
    app.connect('html-page-context', page_context)
    return {'version': '1.0', 'parallel_read_safe': True, 'parallel_write_safe': True}
