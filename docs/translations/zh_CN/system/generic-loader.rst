..
   Copyright (c) 2016, Xilinx Inc.

   This work is licensed under the terms of the GNU GPL, version 2 or later.  See
   the COPYING file in the top-level directory.

通用加载器
--------------

loader 设备允许用户在启动时将多个镜像或数值加载到 QEMU。

将数据写入内存
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
可以在命令行指定内存值，语法如下::

   -device loader,addr=<addr>,data=<data>,data-len=<data-len> \
                   [,data-be=<data-be>][,cpu-num=<cpu-num>]

``<addr>``
  保存数据的地址。

  与 QEMU 其他数值选项一样，默认按十进制解释。十六进制值需加 0x 前缀。

``<data>``
  要写入地址的值，最多 8 字节。

``<data-len>``
  数据长度，单位为字节。指定 data 时必须同时提供此参数。

``<data-be>``
  设为 true 时，以大端字节序写入客户机，默认使用小端。

``<cpu-num>``
  指定使用哪个 CPU 的地址空间加载数据。未指定时使用第一个 CPU。


将 0x8000000e 写入 0xfd1a0104 的示例::

    -device loader,addr=0xfd1a0104,data=0x8000000e,data-len=4

设置 CPU 程序计数器
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

loader 可从命令行设置 CPU 的 PC，语法如下::

     -device loader,addr=<addr>,cpu-num=<cpu-num>

``<addr>``
  要赋给 CPU PC 的值。

  默认按十进制解释，十六进制值需加 0x 前缀。

``<cpu-num>``
  要设置 PC 的 CPU 编号。

将 CPU 0 的 PC 设为 0x8000::

    -device loader,addr=0x8000,cpu-num=0

加载文件
^^^^^^^^^^^^^

loader 还可将文件载入内存，支持 ELF、U-Boot、Intel HEX 和原始镜像。语法为：

    -device loader,file=<file>[,addr=<addr>][,cpu-num=<cpu-num>][,force-raw=<raw>]

``<file>``
  要加载到内存的文件。

``<addr>``
  文件加载地址。原始镜像必需，其他格式忽略。

  默认按十进制解释，十六进制需加 0x 前缀。

``<cpu-num>``
  指定 CPU，可选，具有两个效果：

  * 使用该 CPU 的地址空间加载数据
  * 将该 CPU 的 PC 设为原始文件加载地址，或可执行格式头部指定的入口

  未指定时，通过第一个 CPU 的地址空间加载，不修改任何 CPU 的 PC。

  目前无法只指定地址空间而不同时设置对应 CPU 的 PC。

  因为此选项修改启动 PC，所以只应对启动镜像使用。

``<force-raw>``
  force-raw=on 强制按原始镜像处理文件，可将本来受支持的可执行格式
  当作原始数据加载。


加载 ELF 并由 CPU0 启动的示例::

    -device loader,file=./images/boot.elf,cpu-num=0
