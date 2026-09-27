.. _Sparc32-System-emulator:

Sparc32 系统模拟器
-----------------------

使用 qemu-system-sparc 模拟以下 Sun4m 机器：

-  SPARCstation 4

-  SPARCstation 5

-  SPARCstation 10

-  SPARCstation 20

-  SPARCserver 600MP

-  SPARCstation LX

-  SPARCstation Voyager

-  SPARCclassic

-  SPARCbook

模拟较为完整，支持最多 16 CPU SMP，但 Linux 将可用 CPU 限制为 4 个。

通过 -cpu help 查看 CPU 列表。可选布尔功能可在名称前加 + 启用，
或加 - 禁用，例如 ``-cpu TI-SuperSparc-II,+float128``。

模拟以下 sun4m 外设：

-  IOMMU

-  TCX 或 cgthree 帧缓冲

-  Lance（Am7990）以太网

-  M48T02/M48T08 非易失 RAM

-  从属 I/O：定时器、中断控制器、Zilog 串口、:ref:`keyboard` 和电源/复位逻辑

-  支持硬盘和 CD-ROM 的 ESP SCSI 控制器

-  软驱（SS-600MP 无此设备）

-  CS4231 音频设备（仅 SS-5，尚不可用）

外设数量由架构固定。最大内存取决于机器：SS-5 为 256MB，其他为 2047MB。

自 0.8.2 起，QEMU 使用 OpenBIOS（https://www.openbios.org/），这是采用
GPL v2 的自由、可移植固件，目标是完全符合 IEEE 1275-1994（Open Firmware）。

目前旧版 Solaris 内核不能工作，可能源于 OpenBIOS 与 Solaris 的接口问题。
