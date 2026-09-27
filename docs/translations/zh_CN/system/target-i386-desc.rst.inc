QEMU PC 系统模拟器模拟以下外设：

-  i440FX 主 PCI 桥和 PIIX3 PCI 到 ISA 桥

-  Cirrus CLGD 5446 PCI VGA 显卡，或带 Bochs VESA 扩展的虚拟 VGA 显卡，
   实现硬件级模拟，包括所有非标准模式

-  PS/2 鼠标和键盘

-  两个支持硬盘和 CD-ROM 的 PCI IDE 接口

-  软盘

-  PCI 和 ISA 网卡

-  串口

-  内部或外部 IPMI BMC

-  Creative SoundBlaster 16 声卡

-  ENSONIQ AudioPCI ES1370 声卡

-  Intel 82801AA AC97 兼容声卡

-  Intel HD Audio 控制器和 HDA 编解码器

-  Adlib（OPL2），兼容 Yamaha YM3812 芯片

-  Gravis Ultrasound GF1 声卡

-  CS4231A 兼容声卡

-  PC 扬声器

-  PCI UHCI、OHCI、EHCI 或 XHCI USB 控制器及虚拟 USB 1.1 集线器

支持大量虚拟 CPU 的 SMP，上限取决于配置。

QEMU 使用 SeaBIOS 项目的 PC BIOS 和 Plex86/Bochs LGPL VGA BIOS。

YM3812 模拟由 Tatsuyuki Satoh 提供。

GUS 模拟由 Tibor \"TS\" Schütz 提供，项目为
GUSEMU32（http://www.deinmeister.de/gusemu/）。

GUS 默认与并口共享 IRQ 7，要正常使用，应禁用并口：

.. parsed-literal::

   |qemu_system_x86| dos.img -device gus -parallel none


也可以指定：

.. parsed-literal::

   |qemu_system_x86| dos.img -device gus,irq=5


或其他未占用的 IRQ。

CS4231A 用于 Windows Sound System 和 GUSMAX 产品。

PC 扬声器可通过机器属性 pcspk-audiodev 配置，例如：

.. parsed-literal::

   |qemu_system_x86| some.img \
   -audiodev <backend>,id=<name> \
   -machine pcspk-audiodev=<name>


机器专用选项
~~~~~~~~~~~~~~~~~~~~~~~~

支持以下选项：

- ``x-south-bridge=PIIX3|piix4-isa``：实验性南桥选择，默认为 ``PIIX3``。
