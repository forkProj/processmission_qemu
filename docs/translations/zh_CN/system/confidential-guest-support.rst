机密客户机支持
==========================

传统上，QEMU 等虚拟机监控程序可以完整访问客户机内存及其他状态。因此监控
程序一旦被攻破，任何客户机都可能受影响。多个平台通过硬件或固件机制，为
客户机提供一定保护，防止恶意监控程序访问，这对公有云尤其重要。

这些机制名称和工作方式不同，通常称为安全客户机或机密客户机。本文使用
“机密客户机支持”，以区别于防御其他客户机或网络攻击等安全问题。

运行机密客户机
----------------------------

需要添加两个命令行参数：

1. 用 ``-object`` 创建机密客户机支持对象，类型和参数取决于具体机制。
2. 将机器参数 ``confidential-guest-support`` 设为上述对象 ID。

AMD SEV 示例::

    qemu-system-x86_64 \
        <other parameters> \
        -machine ...,confidential-guest-support=sev0 \
        -object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=1

支持的机制
--------------------

目前支持：

* AMD Secure Encrypted Virtualization（SEV），见 :doc:`i386/amd-memory-encryption`
* Intel Trust Domain Extension（TDX），见 :doc:`i386/tdx`
* POWER Protected Execution Facility（PEF），见 :ref:`power-papr-protected-execution-facility-pef`
* s390x Protected Virtualization（PV），见 :doc:`s390x/protvirt`
* AWS Nitro Enclaves，见 :doc:`nitro`

未来可能支持其他机制。
