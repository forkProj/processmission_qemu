:orphan:

==========================================
loongson3 virt 通用平台（``virt``）
==========================================

``virt`` 机器使用 gpex 主机桥，virt 板上还模拟了若干设备，例如 loongson7a RTC、IOAPIC 和 ACPI 设备等。

支持的设备
-----------------

``virt`` 机器支持：

* Gpex 主机桥
* Ls7a RTC 设备
* Ls7a IOAPIC 设备
* ACPI GED 设备
* Fw_cfg 设备
* PCI/PCIe 设备
* 内存设备
* CPU 设备。类型：la464。

CPU 和机器类型
--------------------

``qemu-system-loongarch64`` 提供 virt 机器模拟。可以指定机器类型为 ``virt``，CPU 类型为 ``la464``。

启动选项
------------

可以通过指定 UEFI BIOS、initrd 和 Linux 内核启动 LoongArch virt 机器。以下步骤说明如何获取这些源代码和二进制文件。

(1) 构建 qemu-system-loongarch64：

.. code-block:: bash

  ./configure --target-list="loongarch64-softmmu"
  make -j8


(2) 设置交叉编译工具：

.. code-block:: bash

  wget https://github.com/loongson/build-tools/releases/download/2022.09.06/loongarch64-clfs-6.3-cross-tools-gcc-glibc.tar.xz
  tar -vxf loongarch64-clfs-6.3-cross-tools-gcc-glibc.tar.xz  -C /opt
  export PATH=/opt/cross-tools/bin:$PATH
  export LD_LIBRARY_PATH=/opt/cross-tools/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH=/opt/cross-tools/loongarch64-unknown-linux-gnu/lib/:$LD_LIBRARY_PATH


注意：需要从 https://github.com/loongson/build-tools 获取最新的交叉编译工具。

(3) 构建 BIOS：

    参见：https://github.com/tianocore/edk2/tree/master/OvmfPkg/LoongArchVirt#readme

注意：要构建 BIOS 的发布版本，请设置 --buildtarget=RELEASE。
      BIOS 文件路径为 Build/LoongArchQemu/RELEASE_GCC5/FV/QEMU_EFI.fd。

(4) 构建内核：

.. code-block:: bash

  git clone https://github.com/loongson/linux.git
  cd linux
  git checkout loongarch-next
  make ARCH=loongarch CROSS_COMPILE=loongarch64-unknown-linux-gnu- loongson3_defconfig
  make ARCH=loongarch CROSS_COMPILE=loongarch64-unknown-linux-gnu- -j32


注意：Linux 源代码的分支为 loongarch-next。
      内核文件为 arch/loongarch/boot/vmlinuz.efi。

(5) 获取 initrd：

  可以使用 busybox 工具和 Linux 模块制作 initrd 文件，也可以从以下位置获取二进制文件：https://github.com/yangxiaojuan-loongson/qemu-binary

.. code-block:: bash

  git clone https://github.com/yangxiaojuan-loongson/qemu-binary


注意：initrd 文件为 ramdisk。

(6) 启动 LoongArch：

.. code-block:: bash

  $ ./build/qemu-system-loongarch64 -machine virt -m 4G -cpu la464 \
      -smp 1 -bios QEMU_EFI.fd -kernel vmlinuz.efi -initrd ramdisk \
      -serial stdio   -monitor telnet:localhost:4495,server,nowait \
      -append "root=/dev/ram rdinit=/sbin/init console=ttyS0,115200" \
      --nographic
