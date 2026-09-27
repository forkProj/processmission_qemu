.. _kconfig:

===============
QEMU 与 Kconfig
===============

QEMU 用途广泛，可为多种目标构建，每个目标又可模拟多个开发板，
不同目标之间还能共享大量代码。
例如 POWER 与 x86 开发板虽然 PCI 主桥不同，却可共用 PCI 网卡模拟；
虽然 SCSI 适配器不同，也可共用 SCSI 磁盘模拟。
Arm、s390 和 x86 都能向客户机提供 virtio-blk，但使用三种不同的客户机接口。

每个目标只启用源码中部分开发板、设备和总线。
因此每个可执行文件只链接源码的一小部分，移除支持该目标所不需要的内容。

QEMU 使用简单的领域专用语言描述组件依赖，主要有两个好处：

* 添加目标和开发板时，无需深入了解硬件模拟子系统架构。
  开发板只需列出所需组件，最终程序就会包含全部依赖，以及用户可添加到该板的设备。

* 用户可方便地构建仅支持部分板或设备的精简版本。
  例如多数目标默认包含 QEMU 支持的全部 PCI 设备，
  但可通过配置排除不需要或不想要的代码，减小二进制文件。

此语言源于 Linux 内核 Kconfig，但大幅简化，且 QEMU 的依赖处理更严格。

与 Linux 不同，它没有编辑配置的用户界面，
而通过源码 configs/ 目录下的各目标文件指定。
原因是 QEMU 构建时通常可以把配置和依赖视为黑盒，
附带默认配置适合几乎所有情况。

Kconfig 语言
------------

Kconfig 在 ``hw/*/Kconfig`` 中定义可配置组件。
这些组件不会以预处理符号形式对 C 代码可见，只对 Makefile 可见。
每个组件定义一个以 ``CONFIG_`` 开头的 Makefile 变量。

所有元素都是布尔类型，y 表示真，n 表示假。定义形式如下::

      config ARM_VIRT
         bool
         imply PCI_DEVICES
         select A15MPCORE
         select ACPI
         select ARM_SMMUV3


config 引入新配置元素。上例使 Makefile 可以访问 CONFIG_ARM_VIRT，值为 y 或 n。

下文出现 <expr> 的位置都可使用布尔表达式，
&&、|| 和 ! 分别表示与、或、非。

bool 类型声明可省略，但建议保留以提高可读性并兼顾未来扩展。
其后可使用以下指令。

**依赖：** ``depends on <expr>``

  定义当前元素的依赖。表达式为假时，强制变量为假。

**反向依赖：** ``select <symbol> [if <expr>]``

  depends on 可强制符号为假，反向依赖则可强制另一符号为真。
  以下示例中，CONFIG_FOO 为真时 CONFIG_BAZ 必然为真::

    config FOO
      select BAZ


  可选条件表达式只有为真时，select 才生效。

  与 Linux 实现不同，QEMU 检测 depends on 与 select 之间的矛盾，
  并阻止构建冲突配置。

**默认值：** ``default <value> [if <expr>]``

  用户未通过 ``configs/*.mak`` 指定其他值，且 select 或 depends on
  未强制其为真或假时，采用默认值。
  value 只能为 y 或 n，不能是任意布尔表达式，但可通过 if 附加生效条件。

  元素可定义任意数量默认值，多个默认值通常附带不同条件。
  多项条件同时满足时，只有最先定义的一项生效。

**反向默认值（弱反向依赖）：** ``imply <symbol> [if <expr>]``

  类似 select，为另一符号提供 y 的下限，但这只是默认值，
  用户仍可在 ``configs/*.mak`` 中设为 n。以下两种写法等价::

    config FOO
      bool
      imply BAZ

    config BAZ
      bool
      default y if FOO


  下一节说明何时使用 imply 或 default y。

编写 Kconfig 文件的准则
-----------------------

QEMU 可配置元素大体分为五类，各类声明依赖的方式不同。

**子系统，总线是其中的特殊情况**

  例如::

    config SCSI
      bool


  子系统总是默认为假，不使用 default，也不出现在 ``configs/*.mak`` 中。
  由其他符号 select 所需子系统。

  子系统有时也 select 自身依赖的子系统或总线。
  例如 DisplayPort 辅助通道总线 AUX 会选择 I2C，因为它也可作为 I2C 主设备。

**设备**

  例如::

    config MEGASAS_SCSI_PCI
      bool
      default y if PCI_DEVICES
      depends on PCI
      select SCSI


  设备最复杂，可通过多种指令配合，使默认配置包含 QEMU 可访问的全部设备。

  设备 depends on 所连接总线，例如 PCI 设备声明 depends on PCI。
  MMIO 设备则通常没有 depends on。
  设备还 select 自己提供的总线，例如 SCSI 适配器声明 select SCSI。
  通常只有至少一个 depends on 时才使用 default y，默认值还可依赖设备组。

  设备也应选择所用可选子系统。
  例如显卡需要构建 EDID 并发布给客户机时，可声明 select EDID。

**设备组**

  例如::

    config PCI_DEVICES
      bool


  设备组便于一次启用或禁用多个设备，适合多个目标都需要切换同一组设备的场景。
  通常无需其他指令，也不用于 Makefile，只作为 default y 的条件。

  当前有 PCI_DEVICES、I2C_DEVICES 和 TEST_DEVICES 三组。
  PCI 设备通常使用 default y if PCI_DEVICES，而非直接 default y，
  使 s390 等开发板可方便地只支持部分设备，例如 VFIO 直通和 virtio-pci。
  I2C_DEVICES 类似，包含用户可能希望在任意开发板 I2C 总线上添加的设备，
  不包括高度板级专用，或需要命令行无法表达的连接方式的设备。
  TEST_DEVICES 则用于生产虚拟机很少使用，但可测试 QEMU 或 KVM 的设备。

**开发板**

  例如::

    config SUN4M
      bool
      default y
      depends on SPARC && !SPARC64
      imply TCX
      imply CG3
      select CS4231
      select ECCMEMCTL
      select EMPTY_SLOT
      select ESCC
      select ESP
      select FDC
      select SLAVIO
      select LANCE
      select M48T59
      select STP2000


  通过 imply 和 select 列出组成设备。
  没有某设备就完全无法启动时使用 select；
  是否能省略设备取决于 QEMU 命令行时使用 imply。
  开发板默认为真，同时用 depends on 限制到适当目标。
  部分目标的某些板不支持硬件虚拟化，还需依赖 TCG。
  常见依赖还包括 FDT 等库，以及可能取反的 TARGET_BIG_ENDIAN。

  为便于使用，各板会列入对应目标的 ``configs/*.mak``。

**内部元素**

  例如::

    config ECCMEMCTL
      bool
      select ECC


  用于组织多个板或设备共用的代码。
  通常通过 select 启用，并继续选择其他元素。
  不出现在 ``configs/*.mak`` 中，往往连 Makefile 也不直接使用。

编写和修改默认配置
------------------

除 hw/ 下的 Kconfig，每个目标还包含 configs/TARGETNAME-softmmu.mak，
将部分变量初始化为非默认值，作为启用设备和子系统的起点。

示例文件如下::

    # Default configuration for alpha-softmmu

    # Uncomment the following lines to disable these optional devices:
    #
    #CONFIG_PCI_DEVICES=n
    #CONFIG_TEST_DEVICES=n

    # Boards:
    #
    CONFIG_DP264=y


第一部分是注释掉的 =n 赋值，告诉用户哪些设备或设备组由开发板隐含启用。
第二部分为 =y 赋值，列出目标支持的开发板。
用户通常通过取消第一部分注释，或注释第二部分来修改默认配置。

也可为 configure 指定 --without-default-devices。
此时除 select 或 .mak 显式启用外，全部默认为 n，
即禁用 default 和 imply 指令。
用户可能需要调整第一部分，例如::

   CONFIG_PCI_DEVICES=y
   #CONFIG_TEST_DEVICES=n


也可只选择设备组的子集。
若不进一步修改 configs/devices/，不带默认设备的系统模拟器
可能只能启动空机器，甚至这也需要命令行指定 --nodefaults。
不带 --nodefaults 的启动允许失败，但不应异常中止。
使用 --without-default-devices 时 make check 失败视为测试代码缺陷：
测试应使用 --nodefaults，并在缺少必需设备时跳过，不能用 select 绕过问题。

当前没有单一位置列出 CONFIG_PCI_DEVICES 和 CONFIG_TEST_DEVICES 的全部可选设备。
未来希望自动生成 .mak，包含全部符号及其用途说明。

``Kconfig.host``
----------------

某些元素依赖 configure 或 meson.build 检测的主机特性，
例如 KVM 是否可用，或主机是否存在某个库。

应在 Kconfig.host 中列出这些符号::

    config TPM
      bool


并在顶层 meson.build 的 host_kconfig 中加入::

    host_kconfig = \
      (have_tpm ? ['CONFIG_TPM=y'] : []) + \
      (host_os == 'linux' ? ['CONFIG_LINUX=y'] : []) + \
      (have_ivshmem ? ['CONFIG_IVSHMEM=y'] : []) + \
      ...
