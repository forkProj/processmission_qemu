.. _tpm-device:

===============
QEMU TPM 设备
===============

客户机侧硬件接口
=============================

TIS 接口
-------------

QEMU TPM 仿真实现了 TPM TIS 硬件接口，遵循可信计算组织的“TCG PC Client
Specific TPM Interface Specification (TIS)”规范，版本 1.3，日期为 2013 年
3 月 21 日。（参见 `TIS specification`_ 或其更新版本。）

TIS 接口向客户机操作系统提供 0xfed40000-0xfed44fff 地址范围的内存映射 I/O 区域。

QEMU 中与 TPM TIS 接口相关的文件：
 - ``hw/tpm/tpm_tis_common.c``
 - ``hw/tpm/tpm_tis_isa.c``
 - ``hw/tpm/tpm_tis_sysbus.c``
 - ``hw/tpm/tpm_tis_i2c.c``
 - ``hw/tpm/tpm_tis.h``

同时提供 ISA 设备和 sysbus 设备。前者用于 pc/q35 机器，后者可在 Arm virt 机器上实例化。

还提供 I2C 设备支持，可在基于 Arm 的仿真机器中实例化。此设备仅支持 TPM 2 协议。

CRB 接口
-------------

QEMU 还实现了 TPM CRB 接口，遵循可信计算组织的“TCG PC Client Platform TPM Profile
(PTP) Specification”规范，族 "2.0"，级别 00，修订版本 01.03 v22，日期为
2017 年 5 月 22 日。（参见 `CRB specification`_ 或其更新版本。）

CRB 接口向客户机操作系统提供 0xfed40000-0xfed40fff 地址范围的内存映射 I/O 区域
（1 个 locality）。

QEMU 中与 TPM CRB 接口相关的文件：
 - ``hw/tpm/tpm_crb.c``

SPAPR 接口
---------------

pSeries（ppc64）机器提供 tpm-spapr 设备模型。

QEMU 中与 SPAPR 接口相关的文件：
 - ``hw/tpm/tpm_spapr.c``

fw_cfg 接口
================

BIOS／固件可读取 ``"etc/tpm/config"`` fw_cfg 条目，以适当配置客户机。

该条目长 6 字节，采用小端格式，内容如下：

.. code-block:: c

    #define TPM_VERSION_UNSPEC          0
    #define TPM_VERSION_1_2             1
    #define TPM_VERSION_2_0             2

    #define TPM_PPI_VERSION_NONE        0
    #define TPM_PPI_VERSION_1_30        1

    struct FwCfgTPMConfig {
        uint32_t tpmppi_address;         /* PPI memory location */
        uint8_t tpm_version;             /* TPM version */
        uint8_t tpmppi_version;          /* PPI version */
    };

ACPI 接口
==============

TPM 设备使用 ACPI ID "PNP0C31" 定义。QEMU 构建 SSDT，并通过 fw_cfg 设备将其传给
客户机。设备描述包含 TIS 接口基地址 0xfed40000 及 MMIO 区域大小（0x5000）。
如果 QEMU 使用 TPM2，还会提供 TPM2 ACPI 表。设备被描述为采用轮询模式，而非
中断模式，主要原因是找不到未使用的 IRQ。

为支持固件（例如 SeaBIOS）写入度量日志，实现了 TCPA 表。该表提供一个 64kb 缓冲区，
供固件写入日志。对于 TPM 2，只有较新版本的 TPM2 表支持度量日志，无需创建 TCPA 表。

TCPA 和 TPM2 ACPI 表遵循可信计算组织的“TCG ACPI Specification”规范，族 "1.2"
和 "2.0"，级别 00，修订版本 00.37。（参见 `ACPI specification`_ 或其更新版本。）

ACPI PPI 接口
------------------

QEMU 为 TPM 1.2 和 TPM 2 提供物理存在接口（PPI）。此接口需要 ACPI 和固件支持。
（参见 `PPI specification`_。）

PPI 允许系统管理员（root）请求在重启时修改 TPM。PPI 规范定义操作请求以及固件
必须执行的动作。系统管理员通过 ACPI 接口将操作请求编号传递给固件，该接口将编号
写入固件已知的内存位置。重启时，固件找到此编号并向 TPM 发送命令。固件将 TPM
结果码和操作请求编号写入内存位置，ACPI 可从中读取并将结果传递给管理员。

PPI 规范定义了一组固件必须或可以实现的操作。ACPI 接口也允许管理员列出受支持的
操作。在 QEMU 中，ACPI 代码由 QEMU 生成，而固件需逐项实现操作支持，不同固件可能
支持不同的子集。因此，QEMU 引入 PPI 虚拟内存设备，让固件指明它支持哪些操作，
ACPI 据此启用受支持的操作并禁用其他操作。此接口位于主内存中，布局如下：

.. list-table::
   :header-rows: 1
   :widths: 15 10 10 65

   * - 字段
     - 长度
     - 偏移
     - 说明
   * - ``func``
     - 0x100
     - 0x000
     - 固件为各个受支持的操作设置值。取值定义见下文。
   * - ``ppin``
     - 0x1
     - 0x100
     - 要使用的 SMI 中断，由固件设置。不支持。
   * - ``ppip``
     - 0x4
     - 0x101
     - 传给 SMM 代码的 ACPI 函数索引，由 ACPI 设置。不支持。
   * - ``pprp``
     - 0x4
     - 0x105
     - 上一次执行操作的结果，由固件设置。取值见函数索引 5。
   * - ``pprq``
     - 0x4
     - 0x109
     - 要执行的操作请求编号。参见规范中的“Physical Presence Interface Operation
       Summary”表。由 ACPI 设置。
   * - ``pprm``
     - 0x4
     - 0x10d
     - 操作请求的可选参数。值取决于具体操作，由 ACPI 设置。
   * - ``lppr``
     - 0x4
     - 0x111
     - 上一次执行的操作请求编号，由固件从 pprq 字段复制。
   * - ``fret``
     - 0x4
     - 0x115
     - SMM 函数返回的结果码。不支持。
   * - ``res1``
     - 0x40
     - 0x119
     - 保留供将来使用。
   * - ``next_step``
     - 0x1
     - 0x159
     - 重启后由固件执行的操作，供固件使用。
   * - ``movv``
     - 0x1
     - 0x15a
     - 内存覆写变量。

``func`` 字段支持以下值，与 ACPI 函数索引 8 使用的值对应。

.. list-table::
   :header-rows: 1
   :widths: 10 90

   * - 值
     - 说明
   * - 0
     - 操作未实现。
   * - 1
     - 操作只能通过固件访问。
   * - 2
     - 固件配置禁止操作系统执行此操作。
   * - 3
     - 允许操作，要求用户在场。
   * - 4
     - 允许操作，不要求用户在场。

表的位置由 fw_cfg 的 ``tpmppi_address`` 字段给出。PPI 内存区域大小为
0x400（``TPM_PPI_ADDR_SIZE``），为将来的更新预留足够空间。

ARM64 virt 上的 PPI
------------------------------

ARM virt 机器按照 `PPI specification`_ 为 ``tpm-tis-device`` 提供 PPI 支持。

x86 TIS 设备的 PPI 内存区域映射到固定地址 ``0xFED45000`` （位于 TIS MMIO 范围内）；
ARM64 sysbus 设备则将 PPI 内存注册为平台总线上的第二个 MMIO 区域。平台总线在
设备插入时动态分配客户机物理地址。ACPI ``_DSM`` 方法及 PPI 操作区域引用此动态
解析的地址。

PPI 由 ``ppi`` 属性控制（默认为 ``on``）::

    -device tpm-tis-device,tpmdev=tpm0,ppi=on

若没有 PPI，Windows 11 ARM64 等客户机操作系统尝试通过 ACPI ``_DSM`` 方法查询
TPM 物理存在能力时，会记录错误。

QEMU 中与 TPM ACPI 表相关的文件：
 - ``hw/i386/acpi-build.c``
 - ``hw/arm/virt-acpi-build.c``
 - ``hw/acpi/tpm.c``
 - ``include/hw/acpi/tpm.h``

TPM 后端设备
===================

TPM 实现分为前端和后端两部分。前端是硬件接口，例如前述 TPM TIS 接口；另一部分
则为 TPM 后端接口。后端接口实现与 TPM 设备的交互，该设备可以是物理设备或仿真设备。
前后端分离使前端可以连接任意可用后端，因此 TIS 接口既能使用直通后端，也能使用
swtpm 后端。

QEMU 中与 TPM 后端相关的文件：
 - ``backends/tpm.c``
 - ``include/system/tpm.h``
 - ``include/system/tpm_backend.h``

QEMU TPM 直通设备
-------------------------------

如果 QEMU 运行在 Linux 主机操作系统上，可以将硬件 TPM 设备提供给单个 QEMU 客户机。
此时，在尝试使用该设备启动 QEMU 之前，用户必须确保其他程序未使用该设备，
例如 /dev/tpm0。

直通驱动使用主机的 TPM 设备发送 TPM 命令并接收响应。此外，它还访问 TPM 设备的
sysfs 条目，以支持命令取消。由于硬件 TPM 的任何状态都不能在主机间迁移，
使用 TPM 直通驱动时会禁用虚拟机迁移。

主机 TPM 设备已由主机固件初始化，因此虚拟固件为初始化设备而发送的某些命令，
例如 ``TPM_Startup()``，会失败。在这种情况下，固件不应使用 TPM。

通常不建议让客户机与主机共享 TPM 设备。主要原因是两个操作系统会访问设备上唯一的
一组资源，例如平台配置寄存器（PCR）。应用程序或内核安全子系统，例如 Linux 完整性
度量架构（IMA），并不预期共享 PCR。

QEMU 中与 TPM 直通设备相关的文件：
 - ``backends/tpm/tpm_passthrough.c``
 - ``backends/tpm/tpm_util.c``
 - ``include/system/tpm_util.h``


使用主机硬件 TPM ``/dev/tpm0``，通过 TPM 直通设备启动 QEMU 的命令行：

.. code-block:: console

  qemu-system-x86_64 -display sdl -accel kvm \
  -m 1024 -boot d -bios bios-256k.bin -boot menu=on \
  -tpmdev passthrough,id=tpm0,path=/dev/tpm0 \
  -device tpm-tis,tpmdev=tpm0 test.img


如果虚拟机中的 Linux 内核内置 TPM TIS 驱动或可将其作为模块加载，以下命令应产生
类似输出（假定直通的是 TPM 2）：

.. code-block:: console

  # dmesg | grep -i tpm
  [    0.012560] ACPI: TPM2 0x000000000BFFD1900 00004C (v04 BOCHS  \
      BXPC     0000001 BXPC 00000001)

  # ls -l /dev/tpm*
  crw-rw----. 1 tss root  10,   224 Sep  6 12:36 /dev/tpm0
  crw-rw----. 1 tss rss  253, 65536 Sep  6 12:36 /dev/tpmrm0

  从 Linux 5.12 开始，sysfs 中提供 TPM 2 的 PCR 条目：
  # find /sys/devices/ -type f | grep pcr-sha
  ...
  /sys/devices/LNXSYSTEM:00/LNXSYBUS:00/MSFT0101:00/tpm/tpm0/pcr-sha256/1
  ...
  /sys/devices/LNXSYSTEM:00/LNXSYBUS:00/MSFT0101:00/tpm/tpm0/pcr-sha256/9
  ...

QEMU TPM 仿真器设备
----------------------------

TPM 仿真器设备通过名为 'swtpm' 的外部 TPM 仿真器发送 TPM 命令并接收响应。
在尝试通过 QEMU 的 TPM 仿真器访问 swtpm 之前，必须先启动 swtpm 程序。

TPM 仿真器实现了用于传输 TPM 命令和响应的命令通道，以及用于发送控制命令的
控制通道。（参见 `SWTPM protocol`_ 规范。）

控制通道用于复位、初始化和迁移 TPM 状态等操作。

swtpm 程序的行为与硬件 TPM 相同，因此需要由 QEMU 虚拟机内运行的固件初始化。
初始化设备的必要步骤之一是向它发送 TPM_Startup 命令。例如，SeaBIOS 已加入
使用此命令初始化 TPM 1.2 或 TPM 2 设备的支持。

QEMU 中与 TPM 仿真器设备相关的文件：
 - ``backends/tpm/tpm_emulator.c``
 - ``backends/tpm/tpm_util.c``
 - ``include/system/tpm_util.h``

以下命令启动 swtpm，使用基于套接字接口的 UnixIO 控制通道。无需以 root 身份运行。

.. code-block:: console

  mkdir /tmp/mytpm1
  swtpm socket --tpmstate dir=/tmp/mytpm1 \
    --ctrl type=unixio,path=/tmp/mytpm1/swtpm-sock \
    --tpm2 \
    --log level=20

启动 QEMU，使用 TPM 仿真器设备与 swtpm 通信的命令行（x86）：

.. code-block:: console

  qemu-system-x86_64 -display sdl -accel kvm \
    -m 1024 -boot d -bios bios-256k.bin -boot menu=on \
    -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis,tpmdev=tpm0 test.img

仿真 pSeries 机器时，使用以下命令行：

.. code-block:: console

  qemu-system-ppc64 -display sdl -machine pseries,accel=kvm \
    -m 1024 -bios slof.bin -boot menu=on \
    -nodefaults -device VGA -device pci-ohci -device usb-kbd \
    -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-spapr,tpmdev=tpm0 \
    -device spapr-vscsi,id=scsi0,reg=0x00002000 \
    -device virtio-blk-pci,bus=pci.0,addr=0x3,drive=drive-virtio-disk0,id=virtio-disk0 \
    -drive file=test.img,format=raw,if=none,id=drive-virtio-disk0

仿真 Arm virt 机器时，使用以下命令行：

.. code-block:: console

  qemu-system-aarch64 -machine virt,gic-version=3,acpi=off \
    -cpu host -m 4G \
    -nographic -accel kvm \
    -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis-device,tpmdev=tpm0 \
    -device virtio-blk-pci,drive=drv0 \
    -drive format=qcow2,file=hda.qcow2,if=none,id=drv0 \
    -drive if=pflash,format=raw,file=flash0.img,readonly=on \
    -drive if=pflash,format=raw,file=flash1.img

仿真 ast2600-evb BMC 机器且要使用连接到 I2C 总线的 TPM 设备时，使用以下命令行：

.. code-block:: console

  qemu-system-arm -M ast2600-evb -nographic \
    -kernel arch/arm/boot/zImage \
    -dtb arch/arm/boot/dts/aspeed-ast2600-evb.dtb \
    -initrd rootfs.cpio \
    -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis-i2c,tpmdev=tpm0,bus=aspeed.i2c.bus.12,address=0x2e

  测试时，使用此命令将驱动加载到正确地址

  echo tpm_tis_i2c 0x2e > /sys/bus/i2c/devices/i2c-12/new_device

如果固件使用 SeaBIOS，按 'ESC' 进入菜单后，应能看到 TPM 菜单项。

.. code-block:: console

  Select boot device:
  1. DVD/CD [ata1-0: QEMU DVD-ROM ATAPI-4 DVD/CD]
  [...]
  5. Legacy option rom

  t. TPM Configuration

如果虚拟机中的 Linux 内核内置 TPM TIS 驱动或可将其作为模块加载，以下命令应产生
类似输出：

.. code-block:: console

  # dmesg | grep -i tpm
  [    0.012560] ACPI: TPM2 0x000000000BFFD1900 00004C (v04 BOCHS  \
      BXPC     0000001 BXPC 00000001)

  # ls -l /dev/tpm*
  crw-rw----. 1 tss root  10,   224 Sep  6 12:36 /dev/tpm0
  crw-rw----. 1 tss rss  253, 65536 Sep  6 12:36 /dev/tpmrm0

  从 Linux 5.12 开始，sysfs 中提供 TPM 2 的 PCR 条目：
  # find /sys/devices/ -type f | grep pcr-sha
  ...
  /sys/devices/LNXSYSTEM:00/LNXSYBUS:00/MSFT0101:00/tpm/tpm0/pcr-sha256/1
  ...
  /sys/devices/LNXSYSTEM:00/LNXSYBUS:00/MSFT0101:00/tpm/tpm0/pcr-sha256/9
  ...

使用 TPM 仿真器进行迁移
===============================

TPM 仿真器支持以下虚拟机迁移类型：

- 虚拟机保存／恢复（迁移到文件）
- 网络迁移
- 创建快照（迁移到 QoW2 或 QED 等存储）

以下命令序列可用于测试虚拟机保存／恢复。

在第一个终端中，使用以下命令启动一个 swtpm 实例：

.. code-block:: console

  mkdir /tmp/mytpm1
  swtpm socket --tpmstate dir=/tmp/mytpm1 \
    --ctrl type=unixio,path=/tmp/mytpm1/swtpm-sock \
    --tpm2 \
    --log level=20

在第二个终端中启动虚拟机：

.. code-block:: console

  qemu-system-x86_64 -display sdl -accel kvm \
    -m 1024 -boot d -bios bios-256k.bin -boot menu=on \
    -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis,tpmdev=tpm0 \
    -monitor stdio \
    test.img

通过虚拟机内的应用程序，验证所连接的 TPM 是否按预期工作。

要保存虚拟机状态，在第二个终端的 QEMU 监视器中使用以下命令：

.. code-block:: console

  (qemu) migrate "exec:cat > testvm.bin"
  (qemu) quit

此时应存在名为 ``testvm.bin`` 的文件，且 swtpm 和 QEMU 进程应已退出。

要测试“虚拟机恢复”，必须使用与之前相同的参数启动 swtpm。如果之前保存的是
TPM 2 [--tpm2]，现在也必须在命令行中再次传入 --tpm2。

在第一个终端中，使用与之前相同的命令行重启 swtpm：

.. code-block:: console

  swtpm socket --tpmstate dir=/tmp/mytpm1 \
    --ctrl type=unixio,path=/tmp/mytpm1/swtpm-sock \
    --log level=20 --tpm2

在第二个终端中，使用额外的 '-incoming' 选项恢复虚拟机状态。

.. code-block:: console

  qemu-system-x86_64 -display sdl -accel kvm \
    -m 1024 -boot d -bios bios-256k.bin -boot menu=on \
    -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis,tpmdev=tpm0 \
    -incoming "exec:cat < testvm.bin" \
    test.img

迁移故障排查
-------------------------

迁移可能因多种原因失败。如果出现问题，请确保命令行遵循以下规则，并尽可能始终
使用相同版本的 QEMU 和 swtpm。

虚拟机保存和恢复：

 - 除恢复虚拟机时使用的 '-incoming' 选项外，QEMU 命令行参数应相同。

 - swtpm 命令行参数应相同。

将虚拟机迁移到 'localhost'：

 - 除目标端的 '-incoming' 选项外，QEMU 命令行参数应相同。

 - 源端和目标端 swtpm 命令行参数应指向两个不同的目录（--tpmstate dir=...），
   尤其是在同一台机器上使用不同版本 libtpms 时。

通过网络迁移虚拟机：

 - 除目标端的 '-incoming' 选项外，QEMU 命令行参数应相同。

 - swtpm 命令行参数应相同。

虚拟机快照：
 - QEMU 命令行参数应相同。

 - swtpm 命令行参数应相同。


此外，swtpm 层面的迁移失败原因可能包括：

 - 源端和目标端的 swtpm 版本不兼容。

   - 可能不支持将 TPM 状态降级。

   - 源端和目标端的 libtpms 使用了不同的编译选项，目标端拒绝接收状态。

 - 源端和目标端使用了不同的迁移密钥，目标端无法解密迁移状态
   （swtpm ... --migration-key ...）。


.. _TIS specification:
   https://trustedcomputinggroup.org/pc-client-work-group-pc-client-specific-tpm-interface-specification-tis/

.. _CRB specification:
   https://trustedcomputinggroup.org/resource/pc-client-platform-tpm-profile-ptp-specification/


.. _ACPI specification:
   https://trustedcomputinggroup.org/tcg-acpi-specification/

.. _PPI specification:
   https://trustedcomputinggroup.org/resource/tcg-physical-presence-interface-specification/

.. _SWTPM protocol:
   https://github.com/stefanberger/swtpm/blob/master/man/man3/swtpm_ioctls.pod
