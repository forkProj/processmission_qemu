# QEMU 中文文档

`docs-zh` 基于本仓库的 `devel` 分支维护中文文档。
官方源码：[QEMU](https://gitlab.com/qemu-project/qemu)，
中文维护仓库：[Process Mission QEMU](https://github.com/processmission/qemu/tree/docs-zh)。

正文在 `docs/translations/zh_CN/` 中以完整 RST 文件维护，目录与 `docs/` 对应，
不使用 PO 文件。手写的 `.rst.inc` 片段也提供译文。
文档树外的 README 和 Python 包说明，按仓库相对路径保存在 `_repository/`，
供逐篇审阅；它们与上游一样不加入 Sphinx 手册目录。
`tests/qapi-schema/doc-good.rst` 是加载测试 schema 的回归测试入口，保留原样。
QAPI、C 源码注释及 `.hx` 文件生成的 API 和命令说明保留原文，
代码、命令输出、标识符和图片也保持原样。
译文遵循原文件的许可证，见 `LICENSE` 和文档的许可证章节。

## 构建

```sh
python3 -m venv .venv-docs
. .venv-docs/bin/activate
python -m pip install -r docs/requirements-zh.txt
python docs/translations/tools/build-zh
python -m http.server 8768 --directory docs/_build/zh/html
```

此流程直接运行 Sphinx，不编译 QEMU、不构建客户机镜像、不执行原生测试。
先以 dummy builder 解析英文源文，建立引用和结构清单，再生成中文 HTML。
构建会检查译文覆盖、章节层级、代码块、图片，以及生成网页的站内链接和锚点。
协议规范中放在字面块内的手写字段解释仍译为中文；这些说明块单独记录原文与译文摘要，
只放行核对过的具体内容，不豁免整篇文档的代码检查。

本地翻译过程可使用 `--allow-incomplete` 预览，发布时禁止使用该选项。
修改构建工具后运行：

```sh
python -m unittest discover -s docs/translations/tools/tests
python docs/translations/tools/build-zh
```

## 审阅与同步

译文初稿需要人工核对技术含义。结构检查不能代替语言审阅。
`docs/translations/zh_CN/_meta/sources.json` 记录基准提交和逐文件摘要，
上游更新时应对照完整英文文章同步译文，并更新摘要。

English 链接优先打开官方主干文档。没有对应上游页面的 devel 机器文档，
链接到本仓库 docs-zh 分支中保留的英文源文；映射记录在 `_meta/english-links.json`。

## 发布

推送 `docs-zh` 后，专用 GitHub Actions 工作流构建并部署中文文档到
<https://processmission.github.io/qemu/>。原有模拟器 CI 对该文档分支不触发。
