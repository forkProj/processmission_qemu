:orphan:

..
   此文件是 qemu.1 手册页的骨架，主要包含对应的 .rst.inc 文件，
   这些内容同时用于手册页和 HTML 手册。

=======================
QEMU 用户文档
=======================

--------
概要
--------

.. parsed-literal::

   |qemu_system| [options] [disk_image]

-----------
说明
-----------

.. include:: target-i386-desc.rst.inc

-------
选项
-------

disk_image 是 IDE 硬盘 0 的原始硬盘镜像。部分目标不需要磁盘镜像。

如果选项参数是包含逗号的任意字符串，例如 "file=my,file" 或 "string=a,b"，
必须将逗号写两次。例如，"-fw_cfg name=z,string=a,,b" 会解析为
"-fw_cfg name=z,string=a,b"。

.. hxtool-doc:: qemu-options.hx

.. include:: keys.rst.inc

.. include:: mux-chardev.rst.inc

-----
注记
-----

.. include:: device-url-syntax.rst.inc

--------
另请参阅
--------

更准确的信息和 Linux 用户模式模拟器调用方式，见 QEMU HTML 文档。
