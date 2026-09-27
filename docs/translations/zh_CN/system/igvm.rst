独立客户虚拟机（IGVM）支持
================================================

IGVM 文件封装在任意虚拟化栈上确定性启动虚拟机所需的全部信息。因此，构建
文件时即可计算机密客户机初始状态的密码学度量，依赖方可通过远程证明验证。

虽然面向机密计算，也可用于普通客户机。单个文件可定义多个平台，例如同时
配置可在 TDX、SEV 和非机密主机运行的虚拟机。

QEMU 通过用户可创建的 igvm-cfg 对象支持它，该对象指定文件名，再由 -machine
引用，以文件配置虚拟机。

机密平台支持通过 ConfidentialGuestSupport 对象提供。若机器有此对象，
IGVM 加载器会用它配置文件指令的隔离属性。

更多 IGVM 信息
---------------------------

格式规范及 Rust、C 库文档链接见项目仓库：

https://github.com/microsoft/igvm

支持的平台
-------------------

目前支持 KVM 下 AMD SEV、SEV-ES、SEV-SNP 机密客户机，也支持非机密客户机。

AMD SEV、SEV-ES、SEV-SNP 的使用限制
------------------------------------------------------------

IGVM 通过指令配置初始状态，并非每种机密客户机都支持全部指令。例如 SEV
不支持加密保存状态区域，因此无法用 IGVM 设置初始 CPU 状态。遇到当前平台
不支持的指令，会报错并中止启动。

支持列表如下。

.. list-table:: SEV、SEV-ES、SEV-SNP 和非机密平台支持的指令
   :widths: 35 65
   :header-rows: 1

   * - IGVM 指令
     - 说明
   * - IGVM_VHT_PAGE_DATA
     - 支持 NORMAL 的零页、已度量页和未度量页，其他页类型报错。
   * - IGVM_VHT_PARAMETER_AREA
     -
   * - IGVM_VHT_PARAMETER_INSERT
     -
   * - IGVM_VHT_VP_COUNT_PARAMETER
     - 将 CPU 数量填入客户机参数页。
   * - IGVM_VHT_ENVIRONMENT_INFO_PARAMETER
     - 将参数页中的 memory_is_shared 设为 1。

.. list-table:: SEV、SEV-ES 和 SEV-SNP 额外支持的指令
   :widths: 25 75
   :header-rows: 1

   * - IGVM 指令
     - 说明
   * - IGVM_VHT_MEMORY_MAP
     - 使用 E820 表条目填充内存映射页。
   * - IGVM_VHT_REQUIRED_MEMORY
     - 确认客户机指定范围内有可用内存。

.. list-table:: SEV-ES 和 SEV-SNP 额外支持的指令
   :widths: 25 75
   :header-rows: 1

   * - IGVM 指令
     - 说明
   * - IGVM_VHT_VP_CONTEXT
     - 支持设置启动 CPU 和其他 CPU 初始状态，但可提供的 VMSA 字段有限，
       详见下文。

通过 VMSA 设置初始 CPU 状态
---------------------------

SEV-ES 和 SEV-SNP 可在 IGVM 中定义 CPU 初始状态，采用 AMD64 架构程序员
手册第二卷表 B-4 [1] 的 VMSA 结构。

QEMU 将其转换为 CPU 状态，启动时由 KVM 同步到客户机 VMSA，并纳入启动度量。
流程和度量详见 :ref:`amd-sev`。

转换过程不得丢失或改变信息，因此只允许以下寄存器字段：

RAX、RCX、RDX、RBX、RBP、RSI、RDI、R8-R15、RSP、RIP、CS、DS、ES、FS、GS、SS、
CR0、CR3、CR4、XCR0、EFER、PAT、GDT、IDT、LDTR、TR、DR6、DR7、RFLAGS、X87_FCW、
MXCSR。

其他字段非零时会报错。

KVM 将 VMSA GPA 硬编码为 0xFFFFFFFFF000。IGVM 定义初始 CPU 状态时，每个
VMSA 的 GPA 必须匹配。

IGVM 中的固件镜像
-------------------------

为机密客户机支持对象指定 IGVM 后，会覆盖默认固件处理：OVMF 等固件应作为
IGVM 载荷，而非通过闪存设备或 -bios 提供。默认 QEMU 固件不会自动填入内存。

同时提供 IGVM 和 -bios 或 pflash，会报错并中止启动。

运行 IGVM 配置的客户机
-------------------------------------

先生成与目标平台兼容的 IGVM 文件。

buildigvm 工具 [2] 可为非机密 x86 以及 SEV、SEV-ES、SEV-SNP 生成示例文件。

生成 AMD SEV-SNP 的例子::

    buildigvm --firmware /path/to/OVMF.fd --output sev-snp.igvm \
              --cpucount 4 sev-snp


运行时添加 igvm-cfg 对象，并从 -machine 引用。

AMD SEV 示例::

    qemu-system-x86_64 \
        <other parameters> \
        -machine ...,confidential-guest-support=sev0,igvm-cfg=igvm0 \
        -object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=1 \
        -object igvm-cfg,id=igvm0,file=/path/to/sev-snp.igvm


参考资料
----------

[1] AMD64 架构程序员手册，第二卷：系统编程
  修订版 3.43
  https://docs.amd.com/v/u/en-US/24593_3.43

[2] buildigvm：生成包含 OVMF 固件的示例 IGVM 文件的工具
  https://gitlab.com/qemu-project/buildigvm
