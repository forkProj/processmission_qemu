QEMU 标准 VGA
========================================

有 ISA 和 PCI 两种变体。

命令行选项：

``-vga std``
   对 -M isapc 选择 ISA，否则选择 PCI。
``-device VGA``
   PCI 变体。
``-device isa-vga``
   ISA 变体。
``-device secondary-vga``
   不提供传统接口的 PCI 变体。


PCI 规范
------------------------

显然仅适用于 PCI 变体。

PCI ID
   ``1234:1111``

PCI 区域 0
   帧缓冲内存，默认 16 MB，可通过 vga_mem_mb 属性调整大小。

PCI 区域 1
   保留，以便未来将帧缓冲 BAR 改为 64 位。

PCI 区域 2
   MMIO BAR，4096 字节，QEMU 1.3 起提供。

PCI ROM 区域
   保存 vgabios，QEMU 0.14 起提供。


无传统接口的变体不含 ROM，使用 ``PCI_CLASS_DISPLAY_OTHER``，
而不是 ``PCI_CLASS_DISPLAY_VGA``。


使用的 I/O 端口
------------------------

无传统接口的 PCI 变体不适用，应使用 MMIO BAR。

``03c0 - 03df``
   标准 VGA 端口。
``01ce``
   Bochs VBE 接口索引端口。
``01cf``
   Bochs VBE 接口数据端口，仅 x86。
``01d0``
   Bochs VBE 接口数据端口。


使用的内存区域
------------------------

``0xe0000000``
  帧缓冲内存，仅 ISA 变体。

PCI 变体过去也在此镜像帧缓冲 BAR；QEMU 0.14 起不再如此，
但 ``-M pc-$old`` 兼容模式除外。


MMIO 区域规范
------------------------

同样仅适用于 PCI 变体。

``0000 - 03ff``
  EDID 数据块。
``0400 - 041f``
  VGA I/O 端口 0x3c0 到 0x3df 的一对一映射。
  支持字访问，字节以小端顺序写入，即先写索引端口，
  因而一次 MMIO 写即可更新索引寄存器，只需一次 VM exit。
``0500 - 0515``
  Bochs dispi 接口寄存器，平坦映射，没有索引／数据端口。
  访问 16 位寄存器时使用 ``(index << 1)`` 作为偏移。
``0600 - 0607``
  QEMU 扩展寄存器，仅 QEMU 2.2+。
  存在这些寄存器时，PCI 修订号至少为 2。寄存器为 32 位。
``0600``
  QEMU 扩展寄存器区域大小，单位字节。
``0604``
  帧缓冲字节序寄存器。
  - ``0xbebebebe`` 表示大端。
  - ``0x1e1e1e1e`` 表示小端。
