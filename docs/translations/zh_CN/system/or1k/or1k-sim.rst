Or1ksim 开发板
==============

QEMU Or1ksim 模拟标准 OpenRISC 开发板模拟器，也就是标准 SoC 配置。

支持的设备
-----------------

 * 16550A UART
 * ETHOC 以太网控制器
 * SMP（通过 ompic 实现 OpenRISC 多核）

启动选项
------------

通过 -kernel 和 -initrd 加载 Linux 内核及可选磁盘镜像，即可启动。

.. code-block:: bash

  $ qemu-system-or1k -cpu or1220 -M or1k-sim -nographic \
        -kernel vmlinux \
        -initrd initramfs.cpio.gz \
        -m 128

Linux 客户机内核配置
""""""""""""""""""""""""""""""""

Linux OpenRISC 的 or1ksim_defconfig 包含所需驱动。要运行 SMP，请选择
simple_smp_defconfig。

硬件配置信息
""""""""""""""""""""""""""""""""""

开发板自动生成并传递设备树 DTB，描述系统设备的地址、中断线及其他配置。

通过 r3 寄存器向客户机操作系统传入 DTB 地址。
