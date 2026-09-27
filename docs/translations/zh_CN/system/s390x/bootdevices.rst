s390x 上的启动设备
=====================

使用 bootindex 参数启动
--------------------------------

对于传统大型机客户机（即 LPAR 或 z/VM 安装），始终需要显式指定用于启动的磁盘（按 s390x 术语称为从其“IPL”；IPL 表示“初始程序加载”）。

因此，在 QEMU 中启动 s390x 客户机时，应始终用 ``bootindex`` 属性标记所需的启动设备，例如::

 qemu-system-s390x -drive if=none,id=dr1,file=guest.qcow2 \
                   -device virtio-blk,drive=dr1,bootindex=1


多个设备可以具有 bootindex。首先尝试从 bootindex 最小的设备进行 IPL。如果第一个设备 IPL 失败，则尝试 bootindex 第二小的设备，以此类推，直到 IPL 成功或没有剩余启动设备可尝试。

要从 CD-ROM ISO 镜像启动（镜像必须包含 El-Torito 启动信息才可启动），建议指定 ``scsi-cd`` 设备，例如::

 qemu-system-s390x -blockdev file,node-name=c1,filename=... \
                   -device virtio-scsi \
                   -device scsi-cd,drive=c1,bootindex=1


注意，必须使用 ``bootindex`` 属性选择启动设备。s390x 不支持 QEMU 旧式的 ``-boot order=...`` 命令，也不支持 ``-boot once=...``。


不使用 bootindex 参数启动
-----------------------------------

QEMU 客户机固件（即所谓的 s390-ccw BIOS）也具备扫描可用块设备的基础支持。因此，如果未使用 ``bootindex`` 属性指定启动设备，它仍有可能自行找到可启动设备，并从中启动客户机操作系统。不过，此扫描算法仍很粗略，可能不完整，因此在许多情况下可能无法发现可启动设备。强烈建议始终通过 ``bootindex`` 属性指定启动设备。

这也意味着应避免使用 ``-hda``、``-cdrom`` 或 ``-drive if=virtio`` 等传统快捷命令，因为这些命令无法指定 ``bootindex``。注意，便捷的 ``-cdrom`` 选项在 s390x 上甚至不会提供真正的（virtio-scsi）CD-ROM 设备。受 QEMU 代码库技术限制影响，该参数实际提供的是 virtio-blk 设备，而通过 ISO 镜像安装 Linux 发行版时，这可能不是合适的设备类型。建议改用 ``-device scsi-cd`` 指定 CD-ROM 设备（如上所述）。


使用 ``loadparm`` 属性选择内核
------------------------------------------------

``s390-ccw-virtio`` 机器支持 ``loadparm`` 参数，可用于选择客户机磁盘上应由 s390-ccw BIOS 启动的内核。启动 QEMU 时，可以这样指定::

 qemu-system-s390x -machine s390-ccw-virtio,loadparm=<string>


使用此参数的第一种方式，是将这里的 ``<string>`` 设为 ``PROMPT``。此时，s390-ccw BIOS 会显示客户机磁盘上已安装内核的列表，并要求用户输入编号选择要启动的内核，类似于启动 QEMU 时指定 ``-boot menu=on`` 选项的效果。注意，只有使用扇区大小为 4k 字节的 DASD 类磁盘镜像时，菜单列表才会显示已安装内核的名称。在扇区大小为 512 字节的普通 SCSI 类磁盘上，磁盘上的 zipl 加载器没有足够空间存储内核名称，因此这里只会显示没有名称的列表。

使用此参数的第二种方式，是指定 0 至 31 范围内的数字。这些数字对应使用 ``PROMPT`` 选项时显示的编号，s390-ccw BIOS 将尝试自动启动与给定编号关联的内核。注意，可以使用 ``0`` 启动默认条目。如果未给机器的 ``loadparm`` 赋值，也会使用默认条目。

默认情况下，机器的 ``loadparm`` 适用于所有启动设备。如果多个设备被分配了 ``bootindex``，而它们需要不同的 ``loadparm``，则可以逐设备分配独立的 ``loadparm``。

使用逐设备 ``loadparm`` 的客户机示例::

  qemu-system-s390x -drive if=none,id=dr1,file=primary.qcow2 \
                   -device virtio-blk,drive=dr1,bootindex=1 \
                   -drive if=none,id=dr2,file=secondary.qcow2 \
                   -device virtio-blk,drive=dr2,bootindex=2,loadparm=3


在此示例中，主启动设备将尝试使用默认条目执行 IPL（因为此设备和机器都没有指定 ``loadparm``）。如果该设备启动失败，次启动设备将尝试使用编号为 3 的条目执行 IPL。

如果机器和设备都指定了 ``loadparm``，则设备值会覆盖机器值。逐设备 ``loadparm`` 值仅用于分配了 ``bootindex`` 的设备。尝试在没有 ``bootindex`` 的情况下启动时，会使用机器的 ``loadparm``。


从网络设备启动
-----------------------------

QEMU 随附的固件包含一个用于 virtio-net-ccw 设备的小型 TFTP 网络引导加载程序。通过网络启动时，``bootindex`` 属性尤其重要。如果此处没有指定 ``bootindex``，网络引导加载程序将不被考虑，网络启动也会失败。要成功进行网络启动，可尝试类似以下的配置::

 qemu-system-s390x -netdev user,id=n1,tftp=...,bootfile=... \
                   -device virtio-net-ccw,netdev=n1,bootindex=1


网络引导加载程序也对 pxelinux.cfg 风格的配置文件提供基础支持。关于如何在 TFTP 服务器上设置配置文件，请参阅 `PXELINUX 配置页面
<https://wiki.syslinux.org/wiki/index.php?title=PXELINUX#Configuration>`__。支持的配置文件条目为 ``DEFAULT``、``LABEL``、``KERNEL``、``INITRD`` 和 ``APPEND`` （更多信息参见 `Syslinux 配置文件语法
<https://wiki.syslinux.org/wiki/index.php?title=Config>`__）。
