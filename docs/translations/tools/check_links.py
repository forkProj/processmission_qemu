# SPDX-FileCopyrightText: Copyright 2026 Process Mission
# SPDX-License-Identifier: MIT

"""Validate files and fragment targets in a generated HTML site."""

from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


class PageLinks(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.identifiers = set()
        self.links = []

    def handle_starttag(self, tag, attrs):
        attributes = dict(attrs)
        if attributes.get('id'):
            self.identifiers.add(attributes['id'])
        key = 'href' if tag in {'a', 'link'} else 'src' if tag in {'img', 'script'} else None
        if key and attributes.get(key):
            self.links.append(attributes[key])


def validate_links(directory: Path):
    root = directory.resolve()
    pages = {}
    for path in root.rglob('*.html'):
        page = PageLinks()
        page.feed(path.read_text(encoding='utf-8'))
        pages[path] = page
    failures = set()
    for path, page in pages.items():
        for link in page.links:
            url = urlsplit(link)
            if url.scheme or url.netloc:
                continue
            target = (path.parent / unquote(url.path)).resolve() if url.path else path
            if not target.is_relative_to(root) or not target.exists():
                failures.add(f'{path.relative_to(root)}: missing file: {link}')
            elif url.fragment and target in pages:
                if unquote(url.fragment) not in pages[target].identifiers:
                    failures.add(f'{path.relative_to(root)}: missing anchor: {link}')
    if failures:
        raise ValueError('Broken local documentation links:\n' + '\n'.join(sorted(failures)))
    print(f'Validated local links and fragment targets in {len(pages)} HTML pages.')
