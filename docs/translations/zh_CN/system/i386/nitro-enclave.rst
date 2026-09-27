'nitro-enclave' 虚拟机（``nitro-enclave``）
===================================================

``nitro-enclave`` 是一种模拟 *AWS Nitro 飞地* 虚拟机的机器类型。`AWS nitro enclaves`_ 是 Amazon EC2 的一项功能，允许从 Amazon EC2 实例创建称为飞地的隔离执行环境，用于处理高度敏感的数据。飞地没有持久存储，也没有外部网络。飞地虚拟机基于 Firecracker microvm，具有一个用于与创建它的父 EC2 实例通信的 vhost-vsock 设备，以及一个用于加密证明的 Nitro 安全模块（NSM）设备。父实例虚拟机的 CID 始终为 3，而飞地虚拟机获得动态 CID。飞地使用 EIF（`Enclave Image Format`_）文件，其中包含启动所需的内核、命令行和一个或多个 ramdisk。

在 QEMU 中，``nitro-enclave`` 是基于 ``microvm`` 的机器类型，这与 AWS Nitro 飞地类似于 `Firecracker`_ microvm 的方式相似。它可以用于通过 QEMU 在本地测试 EIF 文件，而无需运行真实的 AWS Nitro Enclave；后者由于以安全为核心，调试起来可能较为困难。vsock 设备通过 vhost-user-vsock 模拟，因此必须同时运行另一个能够执行用户空间模拟的进程，例如 rust-vmm crate 中的 `vhost-device-vsock`_，vsock 通信才能正常工作。

从源代码构建 QEMU 时，要加入 nitro-enclave 机器支持，必须具备 ``libcbor`` 和 ``gnutls`` 依赖。

.. _AWS nitro enclaves: https://docs.aws.amazon.com/enclaves/latest/user/nitro-enclave.html
.. _Enclave Image Format: https://github.com/aws/aws-nitro-enclaves-image-format
.. _vhost-device-vsock: https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-vsock
.. _Firecracker: https://firecracker-microvm.github.io

使用 nitro-enclave 机器类型
------------------------------------

机器专用选项
~~~~~~~~~~~~~~~~~~~~~~~~

它支持以下机器专用选项：

- nitro-enclave.vsock=string（必需）（vhost-user-vsock 设备将使用的 '-chardev' 选项中字符设备的 ID）
- nitro-enclave.id=string（可选）（设置飞地标识符）
- nitro-enclave.parent-role=string（可选）（设置父实例 IAM 角色 ARN）
- nitro-enclave.parent-id=string（可选）（设置父实例标识符）


运行 nitro-enclave 虚拟机
~~~~~~~~~~~~~~~~~~~~~~~~~~

首先运行 `vhost-device-vsock`__ （或支持 vhost-user-vsock 的类似工具）。下面值为 1 的 forward-cid 选项将飞地虚拟机发起的所有连接转发到主机，而 forward-listen（端口号由 '+' 分隔）用于将主机发起的连接转发到飞地虚拟机::

  $ vhost-device-vsock \
     --vm guest-cid=4,forward-cid=1,forward-listen=9001+9002,socket=/tmp/vhost4.socket


__ https://github.com/rust-vmm/vhost-device/tree/main/vhost-device-vsock#using-the-vsock-backend

接下来在主机上运行必要的应用，使 nitro-enclave 虚拟机应用的 vsock 通信正常工作。例如，nitro-enclave 虚拟机的 init 进程连接到 CID 3，并发送单字节 hello 心跳（0xB7），通知父虚拟机它已启动，同时等待心跳（0xB7）响应。因此，必须在主机上运行监听 9000 端口的 AF_VSOCK 服务器，并在收到心跳后发送心跳响应，飞地虚拟机才能成功启动。应在主机上运行通常在父 EC2 虚拟机中运行的所有应用，以便与飞地虚拟机成功通信。

随后使用以下命令运行 nitro-enclave 虚拟机，其中 ``hello.eif`` 是可用于创建真实 AWS Nitro 飞地虚拟机的 EIF 文件::

  $ qemu-system-x86_64 -M nitro-enclave,vsock=c,id=hello-world \
     -kernel hello-world.eif -nographic -m 4G --enable-kvm -cpu host \
     -chardev socket,id=c,path=/tmp/vhost4.socket


在此示例中，nitro-enclave 虚拟机的 CID 为 4。如果存在连接到飞地虚拟机的应用，应在飞地虚拟机启动后于主机上运行它们。需要修改这些应用，使其连接到 CID 1（而不是飞地虚拟机的 CID），并使用 vhost-device-vsock 的 forward-listen（例如 9001+9002）选项转发它们连接的端口。
