.. _Sparc64-System-emulator:

Sparc64 系统模拟器
-----------------------

使用 ``qemu-system-sparc64`` 模拟 Sun4u（类似 PC 的 UltraSPARC 机器）、
Sun4v（类似 PC 的 T1 机器）或通用 Niagara（T1）机器。Sun4u 模拟已基本完整，
能够以无显示（-nographic）模式运行 Linux、NetBSD 和 OpenBSD。Sun4v 仍在开发。

Niagara T1 模拟器使用 OpenSPARC T1 项目 S10image/ 目录中的固件和操作系统
二进制文件，可启动 disk.s10hw2 Solaris 镜像。文件可从以下位置获取：
http://download.oracle.com/technetwork/systems/opensparc/OpenSPARCT1_Arch.1.5.tar.bz2

::

   qemu-system-sparc64 -M niagara -L /path-to/S10image/ \
                       -nographic -m 256 \
                       -drive if=pflash,readonly=on,file=/S10image/disk.s10hw2

QEMU 模拟以下外设：

-  UltraSparc IIi APB PCI 桥

-  带 VESA Bochs 扩展的 PCI VGA 兼容显卡

-  PS/2 鼠标和键盘

-  M48T59 非易失 RAM

-  PC 兼容串口

-  两个支持硬盘和 CD-ROM 的 PCI IDE 接口

-  软盘
