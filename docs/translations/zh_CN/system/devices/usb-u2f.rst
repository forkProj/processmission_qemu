通用第二因素（U2F）USB 安全密钥设备
============================================

U2F 是开放认证标准，让面向互联网的依赖方为用户提供强健的第二认证因素。

第二因素由实现 U2F 协议的设备提供。USB U2F 密钥是实现该协议的 USB HID。

QEMU 支持主机硬件密钥直通，也支持软件模拟。

``u2f-passthru``
----------------

此设备将主机真实 U2F 密钥连接到客户机，双向转发全部请求。

与普通设备直通不同，专用直通可让多个客户机共享同一安全密钥。

通过 hidraw 指定 Linux /dev/hidrawN 路径：

.. parsed-literal::
   |qemu_system| -usb -device u2f-passthru,hidraw=/dev/hidraw0


不指定时，自动扫描并使用找到的第一个 U2F，需要可用的 libudev：

.. parsed-literal::
   |qemu_system| -usb -device u2f-passthru


``u2f-emulated``
----------------

这是完全软件模拟的设备，使用
`libu2f-emu <https://github.com/Agnoctopus/libu2f-emu>`__。该库实现 FIDO
Alliance 规定全部传输方式的 U2F 设备端协议。

需要四项内容：

 * EC X.509 证书
 * EC 私钥
 * 4 字节计数器
 * 48 字节随机熵

必须以某种方式配置并提供这四项。

假定主机已安装库，有三种配置方式：

 * 临时
 * 设置目录
 * 手动

临时方式最简单，设备为单次生命周期生成全部内容；不传其他选项时默认如此。

.. parsed-literal::
   |qemu_system| -usb -device u2f-emulated


也可用 dir 指定主机目录，其中必须有：

 * certificate.pem：EC X.509 证书
 * private-key.pem：EC 私钥
 * counter：计数器值
 * entropy：48 字节随机熵

.. parsed-literal::
   |qemu_system| -usb -device u2f-emulated,dir=$dir


若文件不在同一目录，也可分别通过以下选项提供路径：

 * ``cert``
 * ``priv``
 * ``counter``
 * ``entropy``

.. parsed-literal::
   |qemu_system| -usb -device u2f-emulated,cert=$DIR1/$FILE1,priv=$DIR2/$FILE2,counter=$DIR3/$FILE3,entropy=$DIR4/$FILE4
