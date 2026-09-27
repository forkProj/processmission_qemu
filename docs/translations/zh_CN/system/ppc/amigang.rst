=======================================================================
AmigaNG 板（``amigaone``、``pegasos1``、``pegasos2``、``sam460ex``）
=======================================================================

这些 PowerPC 机器模拟的板主要用于运行 Amiga 类操作系统（AmigaOS 4、MorphOS 和 AROS），但也能够运行 Linux，本节介绍的就是 Linux 用法。

Eyetech AmigaOne/Mai Logic Teron（``amigaone``）
================================================

``amigaone`` 机器模拟 Eyetech 的 AmigaOne XE 主板，它是重新冠名的 Mai Logic Teron 板，使用修改后的 U-Boot 固件以支持 AmigaOS 4。

模拟的设备
----------------

 * PowerPC 7457 CPU（也可使用 ``-cpu g3, 750cxe, 750fx`` 或 ``750gx``）
 * Articia S 北桥
 * VIA VT82C686B 南桥
 * 兼容 PCI VGA 的显卡（客户机可能需要其他显卡）
 * PS/2 键盘和鼠标
 * 4 KiB NVRAM（使用 ``-drive if=mtd,format=raw,file=nvram.bin`` 保持内容持久化）

固件
--------

启动过程需要固件二进制文件。这是一个采用 GPL 许可的修改版 U-Boot，但其源码已经遗失，因此无法包含在 QEMU 中。二进制文件可从 https://www.hyperion-entertainment.com/index.php/downloads?view=files&parent=28 获取。ROM 镜像位于最后 512kB，可用以下命令提取：

.. code-block:: bash

  $ tail -c 524288 updater.image > u-boot-amigaone.bin


固件中的 BIOS 模拟器无法运行 QEMU 的标准 vgabios，因此需要改用 ``VGABIOS-lgpl-latest.bin``，可从 http://www.nongnu.org/vgabios 下载。

运行 Linux
-------------

以下链接提供了一些可在 ``amigaone`` 机器上工作的 Linux 镜像：https://sourceforge.net/projects/amigaone-linux/files/debian-installer/。运行以下命令启动系统：

.. code-block:: bash

  $ qemu-system-ppc -machine amigaone -bios u-boot-amigaone.bin \
                    -cdrom "A1 Linux Net Installer.iso" \
                    -device ati-vga,model=rv100,romfile=VGABIOS-lgpl-latest.bin


如果出现固件菜单，选择 ``Boot sequence`` → ``Amiga Multiboot Options``，并将 ``Boot device 1`` 设为 ``Onboard VIA IDE CDROM``。然后按 Escape 直到再次显示主界面，再按一次 Escape，在出现的退出菜单中选择 ``Save settings and exit`` 或 ``Use settings for this session only``。将内核加载到内存可能需要较长时间，但最终会启动并显示安装程序。``ati-vga`` 的 RV100 模拟尚不完整，因此只有帧缓冲可用，DRM 和 3D 不可用。

Genesi/bPlan Pegasos（``pegasos1``、``pegasos2``）
==================================================

``pegasos1`` 机器模拟由 Genesi 销售、bPlan 设计的初代 Pegasos（后来标记为 I）。它使用与 ``amigaone`` 相同的 Articia S 北桥，其他部分则与后来的 Pegasos II 大致相同。

``pegasos2`` 机器模拟 Pegasos II，它是为解决 Pegasos I 北桥问题而重新设计的版本。其原理图可从 https://www.powerdeveloper.org/platforms/pegasos/schematics 获取。

模拟的设备
----------------

 * PowerPC 7457 CPU（也可使用 ``-cpu g3`` 或 ``750cxe``）
 * Articia S 北桥（用于 ``pegasos1``）
 * Marvell MV64361 Discovery II 北桥（用于 ``pegasos2``）
 * VIA VT8231 南桥
 * 兼容 PCI VGA 的显卡（客户机可能需要其他显卡）
 * PS/2 键盘和鼠标

固件
--------

Pegasos 板具有兼容 Open Firmware 的 ROM，基于 SmartFirmware，并带有一些未开源的修改，因此 ROM 二进制文件无法包含在 QEMU 中。bPlan 曾提供 Pegasos II 更新程序，可在 `互联网档案馆
<http://web.archive.org/web/20071021223056/http://www.bplan-gmbh.de/up050404/up050404>`_ 找到。可以通过以下命令从中提取 ROM 镜像：

.. code-block:: bash

  $ tail -c +85581 up050404 | head -c 524288 > pegasos2.rom


运行 Linux
-------------

Debian 8.11 的 PowerPC 版本支持 Pegasos II。固件二进制文件中的 BIOS 模拟器无法运行 QEMU 的标准 vgabios，因此需要禁用后者。运行以下命令启动系统：

.. code-block:: bash

  $ qemu-system-ppc -machine pegasos2 -bios pegasos2.rom \
                    -cdrom debian-8.11.0-powerpc-netinst.iso \
                    -device VGA,romfile="" -serial stdio


在固件的 ``ok`` 提示符处输入 ``boot cd install/pegasos``。

也可以使用 QEMU 内置的最小虚拟 Open Firmware（VOF）模拟，在不使用固件 ROM 的情况下直接启动内核；``pegasos1`` 和 ``pegasos2`` 均支持这种方式。为此，从 CD 镜像中提取内核 ``install/powerpc/vmlinuz-chrp.initrd``，然后运行：

.. code-block:: bash

  $ qemu-system-ppc -machine pegasos2 -serial stdio \
                    -kernel vmlinuz-chrp.initrd -append "---" \
                    -cdrom debian-8.11.0-powerpc-netinst.iso


aCube Sam460ex（``sam460ex``）
==============================

``sam460ex`` 机器模拟 aCube 的 Sam460ex 板，基于 AMCC PowerPC 460EX SoC（尽管名称如此，它实际采用 PPC440 CPU 核心）。

固件
--------

该板固件基于较旧的 U-Boot 版本，并作了支持启动 AmigaOS 4 的修改。固件 ROM 随 QEMU 提供。

模拟的设备
----------------

 * PowerPC 460EX SoC
 * M41T80 串行 RTC 芯片
 * Silicon Motion SM501 的显示部分（与真实板上的 SM502 相同）
 * Silicon Image SiI3112 双端口 SATA 控制器
 * USB 键盘和鼠标

运行 Linux
-------------

唯一开箱即用地支持 Sam460ex 的 Linux 发行版是 CruxPPC 2.x。可通过以下命令启动：

.. code-block:: bash

  $ qemu-system-ppc -machine sam460ex -serial stdio \
                    -drive if=none,id=cd,format=raw,file=crux-ppc-2.7a.iso \
                    -device ide-cd,drive=cd,bus=ide.1


aCube 产品页面 https://www.acube-systems.biz/index.php?page=hardware&pid=5 提供了一些其他内核以及启动其他发行版的说明，但尚未测试。
