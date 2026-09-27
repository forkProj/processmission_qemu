.. _ci:

持续集成（CI）
==============

持续集成（CI）要求每次准备提交一组修改时，都构建整个应用，
并执行一套全面的自动化测试 [1]_，包括单元测试、功能测试和其他测试。

QEMU 的大部分 CI 在 GitLab 基础设施上运行，也使用其他 CI 服务完成特定工作。
最新信息及状态见 `项目 Wiki 测试页面 <https://wiki.qemu.org/Testing/CI>`_。

这些测试也作为合并拉取请求之前的准入检查。
准入检查限制代码在测试或部署流程中进入下一阶段，只有获准后才能推进。
批准可以来自人工操作，也可以由一组测试通过来决定 [2]_。

在 QEMU 中，准入流程发生在拉取请求阶段。
项目负责人运行自己的测试集进行批准，测试通过后合并请求。

.. include:: ci-jobs.rst.inc
.. include:: ci-runners.rst.inc

参考资料
--------

.. [1] Humble, Jez & Farley, David (2010). Continuous Delivery:
       Reliable Software Releases Through Build, Test, and Deployment, p. 55.
.. [2] Humble, Jez & Farley, David (2010). Continuous Delivery:
       Reliable Software Releases Through Build, Test, and Deployment, p. 122.
