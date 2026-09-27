Arm RealView 开发板（``realview-eb``、``realview-eb-mpcore``、``realview-pb-a8``、``realview-pbx-a9``）
=======================================================================================================

模拟 Arm RealView 基板的 EB、PB-A8 和 PBX-A9 等变体。由于与引导加载程序的
交互，仅部分 Linux 内核配置能直接运行。

PB-A8 内核应启用 CONFIG_REALVIEW_HIGH_PHYS_OFFSET，并预期 512M RAM。
PBX-A9 应启用 CONFIG_SPARSEMEM、禁用 CONFIG_REALVIEW_HIGH_PHYS_OFFSET，
并预期 1024M RAM。

模拟设备：

-  ARM926E、ARM1136、ARM11MPCore、Cortex-A8 或 Cortex-A9 MPCore CPU

-  Arm AMBA 通用/分布式中断控制器

-  四个 PL011 UART

-  SMC 91c111 或 SMSC LAN9118 以太网适配器

-  PL110 LCD 控制器

-  带 PS/2 键鼠的 PL050 KMI

-  PCI 主桥

-  PCI OHCI USB 控制器

-  带硬盘和 CD-ROM 的 LSI53C895A PCI SCSI 主机总线适配器

-  带 SD 卡的 PL181 多媒体卡接口
