.. _sec_005finvocation:

调用方式
----------

.. parsed-literal::

   |qemu_system| [options] [disk_image]

disk_image 是 IDE 硬盘 0 的原始硬盘镜像。部分目标不需要磁盘镜像。

如果选项参数是包含逗号的任意字符串，例如 "file=my,file" 或 "string=a,b"，
必须将逗号写两次。例如，"-fw_cfg name=z,string=a,,b" 会解析为
"-fw_cfg name=z,string=a,b"。

.. hxtool-doc:: qemu-options.hx

设备 URL 语法
~~~~~~~~~~~~~~~~~

.. include:: device-url-syntax.rst.inc
