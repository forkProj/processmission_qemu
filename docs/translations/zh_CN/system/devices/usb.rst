USB 模拟
-------------

QEMU 可模拟 PCI UHCI、OHCI、EHCI 或 XHCI 控制器，连接虚拟 USB 或真实主机
设备，后者仅适用于部分操作系统。必要时会自动创建并连接虚拟集线器。

USB 控制器
~~~~~~~~~~~~~~~

XHCI 支持
^^^^^^^^^^^^^^^^^^^^^^^

相比 EHCI、UHCI，XHCI 更适合虚拟化，模拟消耗更少资源，尤其是 CPU。
客户机支持时推荐使用，约 2010 年及以后的系统通常支持：

    |qemu_system| -device qemu-xhci

XHCI 支持 USB 1.1、2.0、3.0，只需这一个控制器。系统只有一条 USB 总线时，
添加设备无需指定 bus=。

EHCI 支持
^^^^^^^^^^^^^^^^^^^^^^^

EHCI 支持 USB 2.0，可独立使用，也可搭配 UHCI/OHCI 伴随控制器支持 1.1。
伴随方式更方便，因为同一总线同时支持两个版本，详见下一节。

独立模式也能另外添加 UHCI/OHCI，但每个控制器创建独立总线，设备必须手动
连接到正确控制器。

pc 机器添加 UHCI 最简单的方法是 -usb，将它创建为 PIIX3 的功能，总线名为
usb-bus.0。

EHCI 可用 -device 添加，强烈建议指定 ID，例如 -device usb-ehci,id=ehci，
得到独立总线名 ehci.0。

添加设备时可指定总线，完整示例：

.. parsed-literal::

    |qemu_system| -M pc ${otheroptions}                        \\
        -drive if=none,id=usbstick,format=raw,file=/path/to/image   \\
        -usb                                                        \\
        -device usb-ehci,id=ehci                                    \\
        -device usb-tablet,bus=usb-bus.0                            \\
        -device usb-storage,bus=ehci.0,drive=usbstick


这将 USB 数位板接到 UHCI，大容量存储接到 EHCI。

伴随控制器支持
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

UHCI/OHCI 可通过 masterbus 和 firstport 作为伴随控制器接到 EHCI 总线。
前者指定总线名称，后者指定首端口。通常六端口 EHCI 配三个双端口 UHCI。

docs 中有预设配置文件，可这样使用：

.. parsed-literal::

   |qemu_system| -readconfig docs/config/ich9-ehci-uhci.cfg


随后用 bus=ehci.0 为设备指定总线。

q35 使用 -usb 也会创建类似配置。

.. _Connecting USB devices:

连接 USB 设备
~~~~~~~~~~~~~~~~~~~~~~

用 -device usb-... 或监视器 device_add 连接，支持以下设备：

``usb-mouse``
   虚拟鼠标，启用后覆盖 PS/2 鼠标模拟。

``usb-tablet``
   使用绝对坐标的指针设备，类似触摸屏，因此无需捕获鼠标即可报告位置。
   同样覆盖 PS/2 鼠标。

``usb-storage,drive=drive_id``
   由 drive_id 支持的大容量存储，见 :ref:`disk images`。采用绝大多数
   U 盘使用的 bulk-only 协议。以下连接到 XHCI，后端为 raw 镜像：

   .. parsed-literal::

       |qemu_system| [...]                                   \\
        -drive if=none,id=stick,format=raw,file=/path/to/file.img \\
        -device nec-usb-xhci,id=xhci                              \\
        -device usb-storage,bus=xhci.0,drive=stick


``usb-uas``
   USB Attached SCSI 设备，不自动创建磁盘。需显式添加 scsi-hd 或 scsi-cd，
   并用 -drive 指定后端。一个设备支持多个逻辑单元，以下创建两个磁盘和
   一个光驱：

   .. parsed-literal::

      |qemu_system| [...]                                         \\
       -drive if=none,id=uas-disk1,format=raw,file=/path/to/file1.img  \\
       -drive if=none,id=uas-disk2,format=raw,file=/path/to/file2.img  \\
       -drive if=none,id=uas-cdrom,media=cdrom,format=raw,file=/path/to/image.iso \\
       -device nec-usb-xhci,id=xhci                                    \\
       -device usb-uas,id=uas,bus=xhci.0                               \\
       -device scsi-hd,bus=uas.0,scsi-id=0,lun=0,drive=uas-disk1       \\
       -device scsi-hd,bus=uas.0,scsi-id=0,lun=1,drive=uas-disk2       \\
       -device scsi-cd,bus=uas.0,scsi-id=0,lun=5,drive=uas-cdrom


``usb-bot``
   与 usb-storage 提供相同 bulk-only 接口，但命令行类似 usb-uas，不自动
   创建 SCSI 磁盘。最多 16 个 LUN，编号必须连续。三个设备必须为 0、1、2，
   不能像上述 UAS 示例一样用 0、1、5。

``usb-mtp,rootdir=dir``
   媒体传输协议设备，将 dir 作为客户机可见文件树根目录。

``usb-host,hostbus=bus,hostaddr=addr``
   按总线和地址直通主机设备。

``usb-host,vendorid=vendor,productid=product``
   按厂商和产品 ID 直通。

``usb-wacom-tablet``
   虚拟 Wacom PenPartner，类似 tablet，但额外报告压力，因此可用于 tslib。

``usb-kbd``
   标准 USB 键盘，覆盖已有 PS/2 键盘。

``usb-serial,chardev=id``
   串口转换器，模拟连接到主机字符设备 id 的 FTDI FT232BM。

``usb-braille,chardev=id``
   模拟 Baum 盲文设备 USB 端口。id 引用 -chardev 定义的设备。通常通过
   BrlAPI 将输出显示在 BRLTTY 支持的设备上：

   .. parsed-literal::

      |qemu_system| [...] -chardev braille,id=brl -device usb-braille,chardev=brl


   等效简写为：

   .. parsed-literal::

      |qemu_system| [...] -usbdevice braille


``usb-net[,netdev=id]``
   支持 CDC 以太网和 RNDIS 的网卡，id 引用 -netdev。例如用户模式网络：

   .. parsed-literal::

      |qemu_system| [...] -netdev user,id=net0 -device usb-net,netdev=net0


``usb-ccid``
   智能卡读卡器。

``usb-audio``
   USB 音频设备。

``u2f-{emulated,passthru}``
   :doc:`usb-u2f`

``canokey``
   实现 FIDO2、OpenPGP、PIV 等的开源安全密钥，见 :ref:`canokey`。

物理端口寻址
^^^^^^^^^^^^^^^^^^^^^^^^

默认连接到指定总线的下一个空闲端口，未指定总线则选择可用总线。可通过
port 显式指定客户机看到的端口。UHCI 有两个根端口 1、2，EHCI 有六个 1-6，
模拟 USB 1.1 集线器有八个。

数位板接到 UHCI 端口 1::

        -device usb-tablet,bus=usb-bus.0,port=1


集线器接到端口 2::

        -device usb-hub,bus=usb-bus.0,port=2


虚拟 U 盘接到该集线器端口 4::

        -device usb-storage,bus=usb-bus.0,port=2.4,drive=...


监视器 device_add 同样接受 port。若还需移除，应指定唯一 ID，再用
device_del 引用，例如::

        (qemu) device_add usb-tablet,bus=usb-bus.0,port=1,id=my-tablet
        (qemu) device_del my-tablet


热插入 USB 存储
~~~~~~~~~~~~~~~~~~~~~~~

usb-bot 和 usb-uas 支持热插入，添加时 attached=false，客户机暂不可见。
先接好一个或多个 SCSI 设备，再显式设为 true，流程为：

#. ``device-add usb-bot,id=foo``
#. ``device-add scsi-{hd,cd},bus=foo.0,lun=0``
#. 可选添加更多设备，LUN 1 至 15
#. ``scripts/qmp/qom-set foo.attached = true``

.. _host_005fusb_005fdevices:

Linux 主机上使用真实 USB 设备
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

警告：这是实验功能，使用时 QEMU 会变慢，USB 摄像头等需要实时流传输的设备
尚不支持。

1. 早期 Linux 2.4 上，确认设备未被主机驱动使用。简单方式是将对应模块从
   mydriver.o 改名为 mydriver.o.disabled，禁用它。

2. 确认 /proc/bus/usb 可用，多数发行版默认启用，类似：

   ::

      ls /proc/bus/usb
      001  devices  drivers


3. 只有 root 可直接访问 USB，可选择以 root 启动或修改设备权限。测试时：

   ::

      chown -R myuid /proc/bus/usb


4. 启动 QEMU，在监视器执行：

   ::

      info usbhost
        Device 1.2, speed 480 Mb/s
          Class 00: USB device 1234:5678, USB DISK


   应看到可用设备列表。不要尝试集线器，它不会工作。

5. 添加设备：

   ::

      device_add usb-host,vendorid=0x1234,productid=0x5678


   客户机通常报告新 USB 插入，也可用 -device usb-host,... 达到相同效果。

6. 现在可以在 QEMU 中使用主机设备。

重新启动 QEMU 后，可能必须拔插一次才能恢复，这是一个缺陷。

指定主机设备的 usb-host 属性
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

前例按 vendorid/productid 选择，但并非唯一方式。支持：

``hostbus=<nr>``
  设备所连主机总线号。
``hostaddr=<nr>``
  操作系统分配的设备地址。
``hostport=<str>``
  设备连接的物理端口。
``vendorid=<hexnr>``
  厂商 ID。
``productid=<hexnr>``
  产品 ID。

理论上可以任意组合，实际常用：

- vendorid + productid：指定设备，在主机出现时直通。

- hostbus + hostport：指定物理端口，插入其中的任意设备都直通。

- hostbus + hostaddr：适合临时直通，因为重新插入后地址会改变。

主机 USB 1.1 由 UHCI/OHCI 管理，2.0 由 EHCI 管理。因此同一物理端口可能
因设备速度不同而出现在不同总线上。假设 2.0 是总线 1 端口 1，1.1 是总线 3
端口 1，可将任意插入设备直通并连接正确 QEMU 总线：

.. parsed-literal::

   |qemu_system| -M pc [...]                            \\
        -usb                                                 \\
        -device usb-ehci,id=ehci                             \\
        -device usb-host,bus=usb-bus.0,hostbus=3,hostport=1  \\
        -device usb-host,bus=ehci.0,hostbus=1,hostport=1


usb-host 复位行为属性
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

guest-reset 和 guest-reset-all 控制客户机能否复位主机物理设备，有三种情况：

``guest-reset=false``
  不允许客户机复位。

``guest-reset=true,guest-resets-all=false``
  仅未初始化、尚无 USB 地址时允许复位，通常只允许一次。这是默认行为。

``guest-reset=true,guest-resets-all=true``
  允许随时复位。

这些选项用于应对有缺陷的设备。理论上应可随时复位并重新初始化，但实际
固件可能崩溃，直到断电重插才恢复响应。

最佳方式取决于具体设备，需要试验。默认不工作时，可尝试其他选项观察改善。

记录 USB 传输
^^^^^^^^^^^^^^^^^^^^

所有 USB 设备支持 pcap 流量记录，例如：

``-device usb-mouse,pcap=mouse.pcap``

文件兼容 Linux usbmon，Wireshark 等工具可解码和检查。
