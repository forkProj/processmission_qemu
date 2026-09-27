.. _RX-System-emulator:

RX 系统模拟器
--------------------

使用 ``qemu-system-rx`` 模拟 RX 目标（GDB 模拟器）。该目标模拟以下设备：

-  R5F562N8 MCU

   -  片上内存（ROM 512KB，RAM 96KB）
   -  中断控制单元（ICUa）
   -  1 通道 8 位定时器（TMR0,1）
   -  2 通道比较匹配定时器（CMT0,1）
   -  1 通道串行通信接口（SCI0）

-  16MB 外部内存

以下展示 ``qemu-system-rx`` 的 RX 用法。

从以下位置下载 ``<u-boot_image_file>``：
https://osdn.net/users/ysato/pf/qemu/dl/u-boot.bin.gz

启动 rx-virt 模拟::
  qemu-system-rx -M gdbsim-r5f562n8 -bios <u-boot_image_file>

从以下位置下载 ``kernel_image_file``：
https://osdn.net/users/ysato/pf/qemu/dl/zImage

从以下位置下载 ``device_tree_blob``：
https://osdn.net/users/ysato/pf/qemu/dl/rx-virt.dtb

启动 rx-virt 模拟::
  qemu-system-rx -M gdbsim-r5f562n8 \
      -kernel <kernel_image_file> -dtb <device_tree_blob> \
      -append "earlycon"
