通过 bootindex 属性管理设备启动顺序
====================================================

QEMU 可以告诉支持它的客户机固件（如 x86 PC BIOS），应按什么设备顺序查找
可启动操作系统。最简单的方法是 ``-boot order=``，也可在命令行各块设备或
网络设备上设置 ``bootindex``，获得更灵活的控制。

固件按 bootindex 决定尝试顺序。未设置的设备具有最低优先级，它们之间的
顺序不作保证，但仍可启动。

部分机器，例如 s390x，不支持 ``-boot order=``，必须使用 bootindex。

``-hda``、``-cdrom`` 等简写无法设置 bootindex，必须展开为 ``-drive``
与 ``-device`` 选项对。

示例
-------

假设机器有两张网卡（virtio、e1000）和两个磁盘（IDE、virtio）：

.. parsed-literal::

  |qemu_system| -drive file=disk1.img,if=none,id=disk1 \\
                -device ide-hd,drive=disk1,bootindex=4 \\
                -drive file=disk2.img,if=none,id=disk2 \\
                -device virtio-blk-pci,drive=disk2,bootindex=3 \\
                -netdev type=user,id=net0 \\
                -device virtio-net-pci,netdev=net0,bootindex=2 \\
                -netdev type=user,id=net1 \\
                -device e1000,netdev=net1,bootindex=1

根据上述命令，固件先尝试 e1000 网卡，失败后尝试 virtio 网卡，再失败则
依次尝试 virtio 磁盘和 IDE 磁盘。

限制
-----------

有些固件限制可用于启动的设备。例如 x86 PC BIOS 规范只允许一个可启动
磁盘。如果该磁盘启动失败，不会再试其他磁盘，但仍可尝试软盘或网络。
s390x BIOS 最多尝试 8 个设备，其中磁盘和 virtio-net 数量可以任意组合。

有时固件无法将 QEMU 指定的设备路径映射为启动方法。固件原生支持的设备
通常没有问题，但依赖选项 ROM、且同一 ROM 用于多个设备时，可能无法可靠
指定其中一个。例如 PC BIOS 下，SCSI HBA 连接 target1、target3、target5
三个可启动设备，ROM 为各设备提供启动方法，却无法从方法反向映射具体目标。
这是 PC BIOS 启动规范的局限。

混用 bootindex 与启动顺序参数
------------------------------------------

不要同时使用 bootindex 和 ``-boot order=...`` 或 ``-boot once=...``。
客户机固件通常只支持其中一种，不支持同时使用。混用会导致未定义行为，
很可能无法从预期设备启动。
