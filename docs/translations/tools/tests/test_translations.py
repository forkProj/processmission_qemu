# SPDX-FileCopyrightText: Copyright 2026 Process Mission
# SPDX-License-Identifier: MIT

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class TranslationBuildTests(unittest.TestCase):
    def test_original_anchors_survive_translated_titles(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            translations = root / 'translations/zh_CN'
            (translations / '_templates').mkdir(parents=True)
            (translations / '_static').mkdir()
            english = (
                'Topic\n=====\n\n.. _topic:\n\nIntroduction\n------------\n\n'
                ':ref:`index:introduction` and :ref:`topic`.\n\n'
                '.. code-block:: console\n\n   echo hello\n\n'
                'Field description::\n\n   Offset 0: an explanatory field note\n'
            )
            chinese = (
                '主题\n====\n\n工具链与 SDK_TOOLCHAIN。\n\n.. _topic:\n\n简介\n----\n\n'
                ':ref:`index:introduction` 和 :ref:`topic`。\n\n'
                '.. code-block:: console\n\n   echo hello\n\n'
                'Field description::\n\n   Offset 0: an explanatory field note\n'
            )
            (root / 'index.rst').write_text(english, encoding='utf-8')
            (translations / 'index.rst').write_text(chinese, encoding='utf-8')
            (root / 'conf.py').write_text(
                "extensions = ['sphinx.ext.autosectionlabel', 'qemu_zh']\n"
                "autosectionlabel_prefix_document = True\n"
                "exclude_patterns = ['translations', '_build']\n"
                "rst_prolog = ''\n",
                encoding='utf-8',
            )
            extension = Path(__file__).resolve().parents[3] / 'sphinx'
            environment = dict(os.environ, PYTHONPATH=str(extension))

            def build(language, builder, destination):
                return subprocess.run(
                    [sys.executable, '-m', 'sphinx', '-W', '--keep-going',
                     '-b', builder, '-D', f'language={language}',
                     '-d', str(root / '_build' / (language + '-doctrees')),
                     str(root), str(root / '_build' / destination)],
                    env=environment, cwd=root, text=True,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                )

            reference = build('en', 'dummy', 'reference')
            self.assertEqual(reference.returncode, 0, reference.stdout)
            rendered = build('zh_CN', 'html', 'html')
            self.assertEqual(rendered.returncode, 0, rendered.stdout)
            html = (root / '_build/html/index.html').read_text(encoding='utf-8')
            self.assertIn('id="topic"', html)
            self.assertIn('id="introduction"', html)
            self.assertIn('href="#introduction"', html)
            self.assertIn('href="#id1"', html)
            self.assertIn('简介', html)
            search_index = (root / '_build/html/searchindex.js').read_text(encoding='utf-8')
            for token in ['工具', '具链', 'sdk_toolchain']:
                self.assertIn(token, json.loads(search_index[len('Search.setIndex('):-1])['terms'])

            (translations / 'index.rst').write_text(
                chinese.replace('echo hello', 'echo changed'), encoding='utf-8'
            )
            invalid = build('zh_CN', 'html', 'html')
            self.assertNotEqual(invalid.returncode, 0)
            self.assertIn('code blocks or command output differ', invalid.stdout)

            prose = 'Offset 0: 字段说明'
            localized = chinese.replace('Offset 0: an explanatory field note', prose)
            (translations / 'index.rst').write_text(localized, encoding='utf-8')
            unapproved = build('zh_CN', 'html', 'html')
            self.assertNotEqual(unapproved.returncode, 0)
            (translations / '_meta').mkdir()
            (translations / '_meta/literal-prose.json').write_text(json.dumps({
                'index': {'1': {
                    'source_sha256': hashlib.sha256(b'Offset 0: an explanatory field note').hexdigest(),
                    'translation_sha256': hashlib.sha256(prose.encode()).hexdigest(),
                    'reason': 'Handwritten field description, not executable code.',
                }}
            }), encoding='utf-8')
            approved = build('zh_CN', 'html', 'html')
            self.assertEqual(approved.returncode, 0, approved.stdout)
            (translations / 'index.rst').write_text(
                localized.replace('echo hello', 'echo changed'), encoding='utf-8'
            )
            unrelated_code = build('zh_CN', 'html', 'html')
            self.assertNotEqual(unrelated_code.returncode, 0)
            (translations / 'index.rst').write_text(
                localized.replace('Offset 0:', 'Offset 1:'), encoding='utf-8'
            )
            changed_prose = build('zh_CN', 'html', 'html')
            self.assertNotEqual(changed_prose.returncode, 0)

            (translations / 'index.rst').write_text(
                chinese.replace('简介\n----', '简介\n===='), encoding='utf-8'
            )
            hierarchy = build('zh_CN', 'html', 'html')
            self.assertNotEqual(hierarchy.returncode, 0)
            self.assertIn('heading hierarchy differs', hierarchy.stdout)


if __name__ == '__main__':
    unittest.main()
