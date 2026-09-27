======================================================
QEMU 安全协议与数据模型（SPDM）支持
======================================================

SPDM 提供认证、证明和密钥交换，帮助实现基础设施安全，是 `DMTF`_ 发布的标准。

QEMU 支持连接 SPDM 响应端实现，使外部应用能够仿真设备的响应逻辑。

设置 SPDM 服务器
========================

使用 SPDM 设备时，QEMU 连接实现 SPDM 功能的服务器。

SPDM-Utils
----------

可用 `SPDM Utils`_ 仿真响应端，这是最简单的方法。

SPDM-Utils 是管理、测试和开发 DMTF SPDM 设备的 Linux 应用，
以 Rust 编写，使用 libspdm。

使用 SPDM-Utils 需完成以下步骤，详情见其 README。

 1. `构建 libspdm <https://github.com/westerndigitalcorporation/spdm-utils?tab=readme-ov-file#build-libspdm>`__
 2. `构建 SPDM Utils <https://github.com/westerndigitalcorporation/spdm-utils?tab=readme-ov-file#build-the-binary>`__
 3. `以服务器模式运行 <https://github.com/westerndigitalcorporation/spdm-utils#qemu-spdm-device-emulation>`__

spdm-emu
--------

可用 `spdm emu`_ 建模 SPDM 响应端。

.. code-block:: shell

    $ cd spdm-emu
    $ git submodule init; git submodule update --recursive
    $ mkdir build; cd build
    $ cmake -DARCH=x64 -DTOOLCHAIN=GCC -DTARGET=Debug -DCRYPTO=openssl ..
    $ make -j32
    $ make copy_sample_key # Build certificates, required for SPDM authentication.

证书应符合 PCIe r6.1 第 6.31.3 节，因此需向 openssl.cnf 添加以下内容：

.. code-block::

    subjectAltName = otherName:2.23.147;UTF8:Vendor=1b36:Device=0010:CC=010802:REV=02:SSVID=1af4:SSID=1100
    2.23.147 = ASN1:OID:2.23.147

然后手动重新生成部分证书：

.. code-block:: shell

    $ openssl req -nodes -newkey ec:param.pem -keyout end_responder.key \
        -out end_responder.req -sha384 -batch \
        -subj "/CN=DMTF libspdm ECP384 responder cert"

    $ openssl x509 -req -in end_responder.req -out end_responder.cert \
        -CA inter.cert -CAkey inter.key -sha384 -days 3650 -set_serial 3 \
        -extensions v3_end -extfile ../openssl.cnf

    $ openssl asn1parse -in end_responder.cert -out end_responder.cert.der

    $ cat ca.cert.der inter.cert.der end_responder.cert.der > bundle_responder.certchain.der

也可改用 SPDM-Utils，它会自动生成正确的证书。

随后可启动响应端：

.. code-block:: shell

    $ cd bin
    $ ./spdm_responder_emu --trans PCI_DOE

连接 SPDM NVMe 设备
==============================

SPDM 服务器运行后，即可启动 QEMU 并连接。

对于 NVMe 设备，先准备块存储：

.. code-block:: shell

    $ cd qemu-spdm/linux/image
    $ dd if=/dev/zero of=blknvme bs=1M count=2096 # 2GB NNMe Drive

然后向 QEMU 命令行添加：

.. code-block:: shell

    -drive file=blknvme,if=none,id=mynvme,format=raw \
        -device nvme,drive=mynvme,serial=deadbeef,spdm_port=2323,spdm_trans=doe

此时 QEMU 会尝试连接 SPDM 服务器。

x86_64 应使用 q35 而非默认机器，因此完整命令可能如下：

.. code-block:: shell

    qemu-system-x86_64 -M q35 \
        --kernel bzImage \
        -drive file=rootfs.ext2,if=virtio,format=raw \
        -append "root=/dev/vda console=ttyS0" \
        -net none -nographic \
        -drive file=blknvme,if=none,id=mynvme,format=raw \
        -device nvme,drive=mynvme,serial=deadbeef,spdm_port=2323,spdm_trans=doe

``spdm_trans`` 指定 QEMU 仿真的底层传输类型。PCIe NVMe 控制器支持 doe 和 nvme。
doe 通过 PCIe 扩展能力 Data Object Exchange（DOE）传输 SPDM；
nvme 则使用 NVMe 管理命令 Security Send/Receive。

.. _DMTF:
   https://www.dmtf.org/standards/SPDM

.. _SPDM Utils:
   https://github.com/westerndigitalcorporation/spdm-utils

.. _spdm emu:
   https://github.com/dmtf/spdm-emu

.. _Build libspdm:
   https://github.com/westerndigitalcorporation/spdm-utils?tab=readme-ov-file#build-libspdm

.. _Build SPDM Utils:
   https://github.com/westerndigitalcorporation/spdm-utils?tab=readme-ov-file#build-the-binary

.. _Run it as a server:
   https://github.com/westerndigitalcorporation/spdm-utils#qemu-spdm-device-emulation
