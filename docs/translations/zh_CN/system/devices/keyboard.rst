.. _keyboard:

Sparc32 键盘
----------------
SUN Type 4、5、5c 键盘通过拨码开关选择语言布局。Solaris 用 ioctl 查询
开关值并选择布局，ss5.bin 等 SUN BIOS 也利用它支持部分布局。不过，QEMU
附带的 OpenBIOS 硬编码为美式布局。

可通过 escc.chnA-sunkbd-layout 驱动属性选择，例如：

-global escc.chnA-sunkbd-layout=de

不同键盘有 6 或 5 个拨码开关，最多选择 64 种布局。Solaris 不支持全部
组合，Sun OpenBoot BIOS 支持的更少。

可指定十六进制、十进制数，部分情况也可用语言字符串，例如：

-global escc.chnA-sunkbd-layout=0x2b

-global escc.chnA-sunkbd-layout=43

-global escc.chnA-sunkbd-layout=sv

上述三例均选择瑞典语布局。以下文档表 3-15 说明各开关值对应的键表文件：
https://docs.oracle.com/cd/E19683-01/806-6642/new-43/index.html
对应关系汇总如下：

.. list-table:: escc.chnA-sunkbd-layout 语言选择值
   :widths: 10 10 10
   :header-rows: 1

   * - 十六进制值
     - 十进制值
     - 语言代码
   * - 0x21
     - 33
     - en-us
   * - 0x23
     - 35
     - fr
   * - 0x24
     - 36
     - da
   * - 0x25
     - 37
     - de
   * - 0x26
     - 38
     - it
   * - 0x27
     - 39
     - nl
   * - 0x28
     - 40
     - no
   * - 0x29
     - 41
     - pt
   * - 0x2a
     - 42
     - es
   * - 0x2b
     - 43
     - sv
   * - 0x2c
     - 44
     - fr-ch
   * - 0x2d
     - 45
     - de-ch
   * - 0x2e
     - 46
     - en-gb
   * - 0x2f
     - 47
     - ko
   * - 0x30
     - 48
     - tw
   * - 0x31
     - 49
     - ja
   * - 0x32
     - 50
     - fr-ca
   * - 0x33
     - 51
     - hu
   * - 0x34
     - 52
     - pl
   * - 0x35
     - 53
     - cz
   * - 0x36
     - 54
     - ru
   * - 0x37
     - 55
     - lv
   * - 0x38
     - 56
     - tr
   * - 0x39
     - 57
     - gr
   * - 0x3a
     - 58
     - ar
   * - 0x3b
     - 59
     - lt
   * - 0x3c
     - 60
     - nl-be
   * - 0x3c
     - 60
     - be

并非所有值都有语言代码，be 和 nl-be 对应同一值。未指定属性时，
默认使用 0x21（en-us）。
