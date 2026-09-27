芯片卡接口设备（CCID）
=================================

USB CCID 设备
---------------
USB CCID 实现 CCID 规范，允许连接遵循相同规范的智能卡读卡器。规范信息如下::

  Universal Serial Bus
  Device Class: Smart Card
  CCID
  Specification for
  Integrated Circuit(s) Cards Interface Devices
  Revision 1.1
  April 22rd, 2005


智能卡用于认证、单点登录、公私钥解密和数字签名。不能简单地将客户端读卡器
USB 直通给客户机，因为客户端将失去设备，甚至因“移除”卡而锁屏。此设备则
允许客户端和客户机同时使用智能卡，也支持不依赖物理设备的完全虚拟读卡器
和智能卡。

构建
--------
密码运算和物理卡访问由 libcacard 完成，构建前需安装开发包。

Red Hat/Fedora::

  yum install libcacard-devel


Ubuntu::

  apt-get install libcacard-dev


配置并构建::

  ./configure --enable-smartcard && make


使用 ccid-card-emulated 访问硬件
--------------------------------------
假定当前主机用户通过 libcacard 可正常使用智能卡，QEMU 可作为另一个客户端::

  qemu -usb -device usb-ccid -device ccid-card-emulated


使用 ccid-card-emulated 访问文件证书
----------------------------------------------------------
先创建 CA 和卡证书，只需一次。这里使用 NSS::

  mkdir fake-smartcard
  cd fake-smartcard
  certutil -N -d sql:$PWD
  certutil -S -d sql:$PWD -s "CN=Fake Smart Card CA" -x -t TC,TC,TC -n fake-smartcard-ca
  certutil -S -d sql:$PWD -t ,, -s "CN=John Doe" -n id-cert -c fake-smartcard-ca
  certutil -S -d sql:$PWD -t ,, -s "CN=John Doe (signing)" --nsCertType smime -n signing-cert -c fake-smartcard-ca
  certutil -S -d sql:$PWD -t ,, -s "CN=John Doe (encryption)" --nsCertType sslClient -n encryption-cert -c fake-smartcard-ca


注意，必须恰好有三个卡证书。

使用 certificates 后端::

  qemu -usb -device usb-ccid -device ccid-card-emulated,backend=certificates,db=sql:$PWD,cert1=id-cert,cert2=signing-cert,cert3=encryption-cert


要在客户机使用，先导出 CA::

  certutil -L -r -d sql:$PWD -o fake-smartcard-ca.cer -n fake-smartcard-ca


再在客户机导入::

  certutil -A -d /etc/pki/nssdb -i fake-smartcard-ca.cer -t TC,TC,TC -n fake-smartcard-ca


Linux 客户机可用 CoolKey PKCS #11 模块访问::

  certutil -d /etc/pki/nssdb -L -h all


它会请求 PIN，即之前证书数据库的密码，然后显示三个证书及手动导入的 CA::

  Certificate Nickname                        Trust Attributes
  fake-smartcard-ca                           CT,C,C
  John Doe:CAC ID Certificate                 u,u,u
  John Doe:CAC Email Signature Certificate    u,u,u
  John Doe:CAC Email Encryption Certificate   u,u,u


若未显示，说明 CoolKey 未安装或未注册到 NSS。可通过 Firefox 或命令行注册::

  modutil -dbdir /etc/pki/nssdb -add "CAC Module" -libfile /usr/lib64/pkcs11/libcoolkeypk11.so
  modutil -dbdir /etc/pki/nssdb -list


使用 ccid-card-passthru 访问客户端硬件
--------------------------------------------------
主机上配置设备及适当 chardev::

  qemu -chardev socket,server=on,host=0.0.0.0,port=2001,id=ccid,wait=off \
       -usb -device usb-ccid -device ccid-card-passthru,chardev=ccid


客户端运行构建 QEMU 时生成的 vscclient::

  vscclient <qemu-host> 2001


使用 ccid-card-passthru 访问客户端证书
------------------------------------------------------
这种方式实际用途不大，但可用于调试配置。

按前文准备，随后以下述方式运行 QEMU 和 vscclient。

QEMU 与前述一致，vscclient 在 fake-smartcard 目录运行::

  qemu -chardev socket,server=on,host=0.0.0.0,port=2001,id=ccid,wait=off \
       -usb -device usb-ccid -device ccid-card-passthru,chardev=ccid
  vscclient -e "db=\"sql:$PWD\" use_hw=no soft=(,Test,CAC,,id-cert,signing-cert,encryption-cert)" <qemu-host> 2001



直通协议场景
-----------------------------
下面是 passthru 卡的典型消息交互。usb-ccid 是 USB 设备，启动时默认未连接。
它需要 chardev，并通过该通道传输 cac_card/vscard_common.h 定义的协议。
设备有三种状态：

* 未连接
* 已连接但无卡
* 已连接且有卡

典型交互如下，箭头表示由客户端或客户机发起::

  client event        |    vscclient           |    passthru    |    usb-ccid  |  guest event
  ------------------------------------------------------------------------------------------------
                      |    VSC_Init            |                |              |
                      |    VSC_ReaderAdd       |                |    attach    |
                      |                        |                |              |  sees new usb device.
    card inserted ->  |                        |                |              |
                      |    VSC_ATR             |   insert       |    insert    |  see new card
                      |                        |                |              |
                      |    VSC_APDU            |   VSC_APDU     |              | <- guest sends APDU
  client <-> physical |                        |                |              |
   card APDU exchange |                        |                |              |
   client response -> |    VSC_APDU            |   VSC_APDU     |              |  receive APDU response
                                                      ...
                                      [APDU<->APDU repeats several times]
                                                      ...
     card removed  -> |                        |                |              |
                      |    VSC_CardRemove      |   remove       |   remove     |   card removed
                                                      ...
                                      [(card insert, apdu's, card remove) repeat]
                                                      ...
    kill/quit         |                        |                |              |
      vscclient       |                        |                |              |
                      |    VSC_ReaderRemove    |                |   detach     |
                      |                        |                |              |   usb device removed.


libcacard
---------
ccid-card-emulated 和 vscclient 都使用 libcacard。它实现完全虚拟的 CAC
兼容卡（美国国防部智能卡标准），并用 NSS 获取证书和执行加密。后端可为
真实读卡器和卡，也可为文件中的证书。
