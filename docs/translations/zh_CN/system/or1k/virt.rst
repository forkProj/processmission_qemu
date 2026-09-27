'virt' 通用虚拟平台
===============================

virt 不对应真实硬件，专为虚拟机设计。如果只想运行 Linux 等客户机，
不需要复现具体硬件的特殊行为和限制，推荐使用它。

支持的设备
-----------------

 * PCI/PCIe 设备
 * 8 个 virtio-mmio 传输设备
 * 16550A UART
 * Goldfish RTC
 * 用于关机和重启的 SiFive Test 设备
 * SMP（通过 ompic 实现 OpenRISC 多核）

启动选项
------------

用 -kernel 和 -initrd 加载内核及可选磁盘镜像，例如：

.. code-block:: bash

  $ qemu-system-or1k -cpu or1220 -M or1k-sim -nographic \
        -device virtio-net-device,netdev=user -netdev user,id=user,net=10.9.0.1/24,host=10.9.0.100 \
        -device virtio-blk-device,drive=d0 -drive file=virt.qcow2,id=d0,if=none,format=qcow2 \
        -kernel vmlinux \
        -initrd initramfs.cpio.gz \
        -m 128

Linux 客户机内核配置
""""""""""""""""""""""""""""""""

Linux OpenRISC 的 virt_defconfig 包含 virt 所需驱动。

硬件配置信息
""""""""""""""""""""""""""""""""""

virt 自动生成并传递设备树 DTB，描述设备地址、中断线及其他配置。

通过 r3 寄存器向客户机传入 DTB 地址。
