.. _direct_005flinux_005fboot:

直接启动 Linux
-----------------

本节介绍无需制作完整可启动镜像，直接在 QEMU 中启动 Linux 内核的方法，
非常适合快速测试内核。

语法如下：

.. parsed-literal::

   |qemu_system| -kernel bzImage -drive file=rootdisk.img,format=raw -append "root=/dev/sda"

``-kernel`` 指定内核镜像，``-append`` 提供内核命令行，``-initrd`` 可指定
INITRD 镜像。

``-shim`` 指定 shim.efi。若启动启用了安全启动的 UEFI 固件，并通过
``-kernel`` 让 UEFI 启动指定内核，就需要该选项。

指定后，客户机 UEFI 先加载、验证并运行 shim。它通常由微软签名，因而能被
固件接受。shim 再加载并验证 Linux 内核。内核通常由发行版签名，验证证书
编入 shim，因此 shim 与内核必须来自同一发行版。

安装介质上的 shim.efi 通常位于 ``EFI/BOOT/BOOT{X64,AA64}.EFI``，
``EFI/$distro/`` 中也可能有第二份。

若不需要图形输出，可用 ``-nographic`` 禁用，并将虚拟串口和 QEMU 监视器
重定向到控制台。典型命令为：

.. parsed-literal::

   |qemu_system| -kernel bzImage -drive file=rootdisk.img,format=raw \
                    -append "root=/dev/sda console=ttyS0" -nographic

按 :kbd:`Ctrl+a c` 在串口控制台和监视器间切换，见 :ref:`GUI_keys`。
