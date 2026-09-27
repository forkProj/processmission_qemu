通过 vfio-ccw 直通子通道
===================================

vfio-ccw 基于中介 VFIO 设备基础设施，允许将特定 I/O 子通道及其设备提供给客户机。主机将不再与这些子通道或设备交互。

注意，虽然 vfio-ccw 应当适用于大多数非 QDIO 设备，但实际仅测试过 ECKD DASD。

配置示例
---------------------

步骤 1：配置主机设备
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

每个 mdev 都通过 UUID 标识，因此第一步是获取一个 UUID::

  [root@host ~]# uuidgen
  7e270a25-e163-4922-af60-757fc8ed48c6


注意：建议使用 ``mdevctl`` 工具进行实际的主机设备配置。

要定义与下文相同的设备配置，并使其自动启动，请使用

::

   [root@host ~]# driverctl -b css set-override 0.0.0313 vfio_ccw
   [root@host ~]# mdevctl define -u 7e270a25-e163-4922-af60-757fc8ed48c6 \
                  -p 0.0.0313 -t vfio_ccw-io -a


如果不能或不想使用 ``mdevctl``，请遵循以下手动流程。

* 找到设备的子通道（本例中设备为 ``0.0.2b09``）::

    [root@host ~]# lscss | grep 0.0.2b09 | awk '{print $2}'
    0.0.0313


* 将子通道（本例中为 ``0.0.0313``）从标准 I/O 子通道驱动解绑，再绑定到 vfio-ccw 驱动::

    [root@host ~]# echo 0.0.0313 > /sys/bus/css/devices/0.0.0313/driver/unbind
    [root@host ~]# echo 0.0.0313 > /sys/bus/css/drivers/vfio_ccw/bind


* 创建中介设备（通过 UUID 标识）::

    [root@host ~]# echo "7e270a25-e163-4922-af60-757fc8ed48c6" > \
    /sys/bus/css/devices/0.0.0313/mdev_supported_types/vfio_ccw-io/create


步骤 2：配置 QEMU
~~~~~~~~~~~~~~~~~~~~~~

* 引用已创建的中介设备，并可选择指定呈现给客户机的设备 ID（这里为 ``fe.0.1234``，在客户机中最终显示为 ``0.0.1234``）::

    -device vfio-ccw,devno=fe.0.1234,sysfsdev=\
    /sys/bus/mdev/devices/7e270a25-e163-4922-af60-757fc8ed48c6


* 启动客户机。此时应当可以使用该设备（这里为 ``0.0.1234``）::

    [root@guest ~]# lscss -d 0.0.1234
    Device   Subchan.  DevType CU Type Use  PIM PAM POM  CHPID
    ----------------------------------------------------------------------
    0.0.1234 0.0.0007  3390/0e 3990/e9      f0  f0  ff   1a2a3a0a 00000000
    [root@guest ~]# chccwdev -e 0.0.1234
    Setting device 0.0.1234 online
    Done
    [root@guest ~]# dmesg -t
    (...)
    dasd-eckd 0.0.1234: A channel path to the device has become operational
    dasd-eckd 0.0.1234: New DASD 3390/0E (CU 3990/01) with 10017 cylinders, 15 heads, 224 sectors
    dasd-eckd 0.0.1234: DASD with 4 KB/block, 7212240 KB total size, 48 KB/track, compatible disk layout
    dasda:VOL1/  0X2B09: dasda1
