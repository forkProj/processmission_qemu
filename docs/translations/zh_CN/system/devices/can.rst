CAN 总线模拟支持
=========================
CAN 总线模拟允许通过一条或多条总线，将多个模拟 CAN 控制器连接起来，连接
由控制器设备的 canbus 参数指定。各总线还可连接主机 CAN API，目前仅支持
Linux SocketCAN。

总线概念是通用的，可以实现不同 CAN 控制器。

最初实现的是 SJA1000，它使用广泛，大多数操作系统都有完善驱动。

首个实现选择 PCI 扩展卡，因为此类设备容易接入 x86、PowerPC、Arm 等不同
CPU 架构的系统。

2020 年，Jan Charvat 的本科毕业论文加入了 CTU CAN FD 模型。这是完整开放
源码、设计和硬件的方案，核心设计者为 Ondrej Ille，由 CTU 及包括大众汽车
子公司在内的多家公司提供资金支持。

项目最初由 Jin Yang 在 RTEMS GSoC 2013 名额下、在团队指导中启动，原计划
为 RTEMS 提供通用 CAN 子系统。但缺少统一的代码和 RTEMS 测试环境，因此
目标调整为提供完整模拟测试环境，该 GSoC 名额用于 QEMU CAN 硬件模拟。

基于 SJA1000 的开发板用法
----------------------------------------------------------
编译启用 CAN PCI 支持后，可选择以下开发板。

(1) Kvaser PCI CAN-S，单 SJA1000 通道。启动选项::

    -object can-bus,id=canbus0
    -device kvaser_pci,canbus=canbus0


添加 can-host-socketcan 对象，连接主机 CAN 总线::

    -object can-host-socketcan,id=canhost0,if=can0,canbus=canbus0


(2) PCM-3680I PCI，双 SJA1000 通道::

    -object can-bus,id=canbus0
    -device pcm3680_pci,canbus0=canbus0,canbus1=canbus0


另一个示例::

    -object can-bus,id=canbus0
    -object can-bus,id=canbus1
    -device pcm3680_pci,canbus0=canbus0,canbus1=canbus1


(3) MIOe-3680 PCI，双 SJA1000 通道::

    -device mioe3680_pci,canbus0=canbus0


kvaser_pci 模型兼容主线 Linux 的同名驱动，并已测试。测试环境主机和客户机
均使用 Linux 4.9。

qemu-system-x86_64 示例::

    qemu-system-x86_64 -accel kvm -kernel /boot/vmlinuz-4.9.0-4-amd64 \
      -initrd ramdisk.cpio \
      -virtfs local,path=shareddir,security_model=none,mount_tag=shareddir \
      -object can-bus,id=canbus0 \
      -object can-host-socketcan,id=canhost0,if=can0,canbus=canbus0 \
      -device kvaser_pci,canbus=canbus0 \
      -nographic -append "console=ttyS0"


qemu-system-arm 示例::

    qemu-system-arm -cpu arm1176 -m 256 -M versatilepb \
      -kernel kernel-qemu-arm1176-versatilepb \
      -hda rpi-wheezy-overlay \
      -append "console=ttyAMA0 root=/dev/sda2 ro init=/sbin/init-overlay" \
      -nographic \
      -virtfs local,path=shareddir,security_model=none,mount_tag=shareddir \
      -object can-bus,id=canbus0 \
      -object can-host-socketcan,id=canhost0,if=can0,canbus=canbus0 \
      -device kvaser_pci,canbus=canbus0,host=can0 \


主机 CAN 接口必须配置正确比特率并启用。模拟设备设置不会通过总线传播到
物理主机设备。以下配置 1 Mbit/s::

  ip link set can0 type can bitrate 1000000
  ip link set can0 up


主机也可用仅本地可见的虚拟接口替代物理接口::

  ip link add dev can0 type vcan


可用 can-utils 中的 candump 分析流量::

  candump can0


CTU CAN FD 支持示例
---------------------------
该开源内核支持 CAN FD。SocketCAN 接口具有 CAN FD 能力时，FD 帧也会传到主机。

目前提供 PCIe 开发板模拟，设备标识为 ctucan_pci，默认包含两个 CTU CAN FD
内核。

以下将虚拟线路 canbus0-bus 同时接到主机 SocketCAN 和模拟 PCI 卡上的两个
内核，假定主机已按上一节配置::

  qemu-system-x86_64 -enable-kvm -kernel /boot/vmlinuz-4.19.52+ \
      -initrd ramdisk.cpio \
      -virtfs local,path=shareddir,security_model=none,mount_tag=shareddir \
      -vga cirrus \
      -append "console=ttyS0" \
      -object can-bus,id=canbus0-bus \
      -object can-host-socketcan,if=can0,canbus=canbus0-bus,id=canbus0-socketcan \
      -device ctucan_pci,canbus0=canbus0-bus,canbus1=canbus0-bus \
      -nographic


客户机 Linux 中设置控制器::

  insmod ctucanfd.ko || modprobe ctucanfd
  insmod ctucanfd_pci.ko || modprobe ctucanfd_pci

  for ifc in /sys/class/net/can* ; do
    if [ -e  $ifc/device/vendor ] ; then
      if ! grep -q 0x1760 $ifc/device/vendor ; then
        continue;
      fi
    else
      continue;
    fi
    if [ -e  $ifc/device/device ] ; then
       if ! grep -q 0xff00 $ifc/device/device ; then
         continue;
       fi
    else
      continue;
    fi
    ifc=$(basename $ifc)
    /bin/ip link set $ifc type can bitrate 1000000 dbitrate 10000000 fd on
    /bin/ip link set $ifc up
  done


例如在客户机运行测试::

  candump can1


主机发送普通 CAN::

  cangen can0


发送不切换比特率的 CAN FD::

  cangen can0 -f


发送切换比特率的 CAN FD::

  cangen can0 -b


也可以反向测试，由客户机生成消息、主机捕获，或采用其他组合。

SabreLite 开发板 FlexCAN 模拟用法
-------------------------------------------------------------------
通过机器属性传入总线 ID，连接 FlexCAN：

* canbus0 连接 flexcan1
* canbus1 连接 flexcan2

上游 Linux SabreLite 设备树仅启用 flexcan1。

以下将两个 FlexCAN 都连接到 qcan0，并桥接主机 can0::

  qemu-system-arm -M sabrelite -smp 4 -m 1G \
    -object can-bus,id=qcan0 \
    -machine canbus0=qcan0 -machine canbus1=qcan0 \
    -object can-host-socketcan,if=can0,canbus=qcan0,id=qcan0-socketcan \
    -kernel ... -dtb ... -initrd ...


客户机 Linux 中的 FlexCAN 比特率值会被忽略，但仍必须通过 ip 命令设置。

其他资源
------------------------

 (1) `捷克技术大学电气工程学院 CAN 项目 <http://canbus.pages.fel.cvut.cz>`_
 (2) `捷克技术大学 can-pci 开发分支仓库 <https://gitlab.fel.cvut.cz/canbus/qemu-canbus>`_
 (3) `RTEMS 项目介绍 <https://devel.rtems.org/wiki/Developer/Simulators/QEMU/CANEmulation>`_
 (4) `RTLWS 2015 关于项目及 CANopen 模拟的论文 <http://cmp.felk.cvut.cz/~pisa/can/doc/rtlws-17-pisa-qemu-can.pdf>`_
 (5) `LinuxDays 2017：实时控制中的 GNU/Linux、CAN 和 CANopen（含更新的 RTLWS 2015 内容） <https://www.linuxdays.cz/2017/video/Pavel_Pisa-CAN_canopen.pdf>`_
 (6) `Linux SocketCAN 工具 <https://github.com/linux-can/can-utils>`_
 (7) `CTU CAN FD：VHDL、Linux 驱动和测试工具等 <https://gitlab.fel.cvut.cz/canbus/ctucanfd_ip_core>`_
 (8) `CTU CAN FD 内核数据手册 <http://canbus.pages.fel.cvut.cz/ctucanfd_ip_core/doc/Datasheet.pdf>`_
 (9) `CTU CAN FD 内核系统架构 <http://canbus.pages.fel.cvut.cz/ctucanfd_ip_core/doc/System_Architecture.pdf>`_
 (10) `CTU CAN FD 驱动文档 <https://canbus.pages.fel.cvut.cz/ctucanfd_ip_core/doc/linux_driver/build/ctucanfd-driver.html>`_
 (11) `Intel/Altera Cyclone IV 开发板 PCIe 接口集成 <https://gitlab.fel.cvut.cz/canbus/pcie-ctu_can_fd>`_
