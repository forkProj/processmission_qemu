.. _canokey:

CanoKey QEMU
------------

CanoKey [1]_ 是开源安全密钥，支持：

* 带 Ed25519 和 HMAC-secret 的 U2F/FIDO2
* 带 RSA4096、Ed25519 等的 OpenPGP Card V3.4 [2]_
* PIV（NIST SP 800-73-4）
* HOTP/TOTP
* NDEF

这些跨平台功能位于 canokey-core [3]_。

各平台实现包括硬件和虚拟卡：

* CanoKey STM32 [4]_
* CanoKey Pigeon [5]_
* 虚拟卡 CanoKey USB/IP
* 虚拟卡 CanoKey FunctionFS

QEMU 实现了另一种虚拟卡，以 USB 设备呈现给客户机。

使用与硬件密钥相同的软件配置，客户机就能像真实插入设备一样使用全部功能。

调试便利包括：

* libcanokey-qemu 提供内部调试输出
* CanoKey QEMU 提供跟踪事件
* USB 栈支持 pcap，可捕获并分析客户机与密钥之间的数据包

因此：

* FIDO2、OpenPGP 等软件开发者可查看密钥内部行为
* 密钥开发者可方便捕获和分析 USB 交互

虚拟卡也便于在 CI 测试处理安全密钥的代码。

构建
========

需要 libcanokey-qemu。

.. code-block:: shell

    git clone https://github.com/canokeys/canokey-qemu
    mkdir canokey-qemu/build
    pushd canokey-qemu/build


若要安装到其他位置，在下方 CMake 命令添加
``-DCMAKE_INSTALL_PREFIX=/path/to/your/place``。

.. code-block:: shell

    cmake ..
    make
    make install # may need sudo
    popd


然后配置并构建 QEMU：

.. code-block:: shell

    # depending on your env, lib/pkgconfig can be lib64/pkgconfig
    export PKG_CONFIG_PATH=/path/to/your/place/lib/pkgconfig:$PKG_CONFIG_PATH
    ./configure --enable-canokey && make


使用 CanoKey QEMU
==================

全部数据保存在启动参数指定的主机文件中。

.. parsed-literal::

    |qemu_system| -usb -device canokey,file=$HOME/.canokey-file


注意妥善保管，该文件可能包含私钥。

首次使用时创建并初始化，之后读取已有文件。

客户机启动后可检查 USB 设备。例如 Linux 中运行 lsusb：

.. code-block:: shell

    $ lsusb
    Bus 001 Device 002: ID 20a0:42d4 Clay Logic CanoKey QEMU


按 [6]_ 配置，管理控制台位于 [7]_。

调试
=========

由 libcanokey-qemu.so 和 QEMU 内的 canokey.c 组成，前者实现密钥核心功能，
后者处理平台相关的 USB 数据包。

要跟踪内部行为，构建库时添加 -DQEMU_DEBUG_OUTPUT=ON：

.. code-block:: shell

    cmake .. -DQEMU_DEBUG_OUTPUT=ON


跟踪 canokey.c 事件：

.. parsed-literal::

    |qemu_system| --trace "canokey_*" \\
        -usb -device canokey,file=$HOME/.canokey-file


捕获客户机与主机的 USB 数据包：

.. parsed-literal::

    |qemu_system| -usb -device canokey,file=$HOME/.canokey-file,pcap=key.pcap


限制
===========

库最初为嵌入式设计，具有数十个全局变量，因此一个 QEMU 实例不能运行多个
CanoKey，即不能这样使用：

.. parsed-literal::

    |qemu_system| -usb -device canokey,file=$HOME/.canokey-file \\
         -device canokey,file=$HOME/.canokey-file2


此外，数据文件没有锁，两个实例不能同时读取同一个 canokey-file。

参考资料
==========

.. [1] `<https://canokeys.org>`_
.. [2] `<https://docs.canokeys.org/userguide/openpgp/#supported-algorithm>`_
.. [3] `<https://github.com/canokeys/canokey-core>`_
.. [4] `<https://github.com/canokeys/canokey-stm32>`_
.. [5] `<https://github.com/canokeys/canokey-pigeon>`_
.. [6] `<https://docs.canokeys.org/>`_
.. [7] `<https://console.canokeys.org/>`_
