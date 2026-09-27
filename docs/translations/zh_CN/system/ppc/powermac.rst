PowerMac 系列板（``g3beige``、``mac99``）
==================================================================

使用 ``qemu-system-ppc`` 可执行文件模拟完整的 PowerMac PowerPC 系统。

- ``g3beige``              基于 Heathrow 的 PowerMac
- ``mac99``                基于 Mac99 的 PowerMac

支持的设备
-----------------

QEMU 模拟以下 PowerMac 外设：

 *  UniNorth 或 Grackle PCI 桥
 *  带 VESA Bochs 扩展的兼容 PCI VGA 显卡
 *  两个 PMAC IDE 接口，支持硬盘和 CD-ROM
 *  NE2000 PCI 适配器
 *  非易失性 RAM
 *  VIA-CUDA，带 ADB 键盘和鼠标


缺少的设备
---------------

 * 待确定

固件
--------

从 0.9.1 版本开始，QEMU 为 g3beige 和 mac99 PowerMac 以及 40p 机器使用 OpenBIOS：https://www.openbios.org/。OpenBIOS 是自由的（GPL v2）可移植固件实现，目标是实现完全符合 IEEE 1275-1994（称为 Open Firmware）的固件。
