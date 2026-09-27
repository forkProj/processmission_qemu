.. SPDX-License-Identifier: GPL-2.0-or-later

RISC-V 服务器平台参考板（``riscv-server-ref``）
=============================================================

RISC-V 服务器平台规范 `spec`_ 定义了一组标准化的硬件和软件能力，操作系统、虚拟机监控器等可移植系统软件可以依赖这些能力在 RISC-V 服务器平台上存在。此机器旨在模拟该规范，为固件和操作系统开发及测试提供环境。

此板引入时，`spec`_ 的版本为 1.0。新规范版本可能会促使模拟实现修订；该实现将尽力始终匹配最新规范。如果模拟变化不向后兼容，将引入版本控制机制（可能通过机器属性），允许旧软件使用旧规范版本运行。

riscv-server-ref 板的主要功能包括：

* IOMMU 平台设备（riscv-iommu-sys）
* AIA
* PCIe AHCI
* PCIe 网卡
* 没有 virtio mmio 总线
* 没有 fw_cfg 设备
* 没有 ACPI 表
* 最小化的设备树节点

此参考板有多种使用方式。符合规范的方式是使用 EDK2 镜像和 TPM 设备。此板已使用依赖外部 ``swtpm`` 模拟器的 ``tpm-tis`` TPM 设备进行测试。有关此设备的更多使用信息，请参阅 `tpm`_。

要将此板与 tpm-tis 设备配合使用，首先在 shell 中启动 ``swtpm`` 进程（``log`` 参数可选）：

.. code-block:: bash

    $ mkdir /tmp/mytpm1
    $ swtpm socket --tpmstate dir=/tmp/mytpm1 \
            --ctrl type=unixio,path=/tmp/mytpm1/swtpm-sock \
            --tpm2 \
            --log level=20


然后启动 QEMU：

.. code-block:: bash

 qemu-system-riscv64 -M riscv-server-ref \
     -bios fw_dynamic.bin \
     -kernel EDK2.fd \
     -drive file=nvme_disk.ext2,format=raw,id=hd0,if=none \
     -device ahci,id=ahci \
     -device ide-hd,drive=hd0,bus=ahci.0 \
     -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
     -tpmdev emulator,id=tpm0,chardev=chrtpm \
     -device tpm-tis-device,tpmdev=tpm0 \
     -nographic



.. _spec: https://github.com/riscv-non-isa/riscv-server-platform
.. _tpm: https://qemu-project.gitlab.io/qemu/specs/tpm.html
