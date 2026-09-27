Xen 设备模拟后端 (``xenpvh``)
=========================================

该机器较为特殊，QEMU 仅作为 IOREQ 服务器向 Xen 注册并连接，虚拟机本身
仍由 Xen 工具控制。

启用 TPM 时，还会在用户指定基址创建 tpm-tis-device，添加 TPM 模拟器，
并通过 chardev 套接字连接主机 swtpm，从而为客户机域提供 TPM 功能。

TPM 用法及 Linux swtpm 安装方法见 :ref:`tpm-device`。

主机启动 swtpm 示例：

.. code-block:: console

    mkdir /tmp/vtpm2
    swtpm socket --tpmstate dir=/tmp/vtpm2 \
      --ctrl type=unixio,path=/tmp/vtpm2/swtpm-sock &

启动 xenpvh 并连接 Xen 的示例：

.. code-block:: console

    qemu-system-aarch64 -xen-domid 1 \
      -chardev socket,id=libxl-cmd,path=qmp-libxl-1,server=on,wait=off \
      -object monitor-qmp,id=qmp0,chardev=libxl-cmd \
      -chardev socket,id=libxenstat-cmd,path=qmp-libxenstat-1,server=on,wait=off \
      -object monitor-qmp,id=qmp1,chardev=libxenstat-cmd \
      -xen-attach -name guest0 -vnc none -display none -nographic \
      -machine xenpvh -m 1301 \
      -chardev socket,id=chrtpm,path=tmp/vtpm2/swtpm-sock \
      -tpmdev emulator,id=tpm0,chardev=chrtpm -machine tpm-base-addr=0x0C000000

上述命令最后两行通过 chardev 套接字连接 swtpm。
