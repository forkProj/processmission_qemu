AMD Versal Virt（``amd-versal-virt``、``amd-versal2-virt``）
============================================================

AMD Versal 是异构多核 SoC（片上系统）系列，将处理系统（PS）中的传统硬化 CPU 和 I/O 外设，与运行时可编程 FPGA 逻辑（PL）及人工智能引擎（AIE）结合。

QEMU 实现以下 Versal SoC 变体：

- Versal（``amd-versal-virt`` 机器，保留 ``xlnx-versal-virt`` 别名以向后兼容）
- Versal Gen 2（``amd-versal2-virt`` 机器）

更多详情：
https://www.amd.com/en/products/adaptive-socs-and-fpgas/versal.html

Versal SoC 系列共享同一架构，但不同型号具有不同速度等级、PL 数量及其他差异。

QEMU 中的 AMD Versal Virt 板是虚拟板模型（现实中不存在），具有不受 I/O 限制的虚拟 Versal SoC。目前支持以下核心和设备。

Versal
""""""

已实现的 CPU 核心：

- 两个 ACPU（Arm Cortex-A72），以及相应 GICv3 和 ITS
- 两个 RCPU（Arm Cortex-R5F），以及相应 GICv2

已实现的设备：

- 两个 UART（Arm PL011）
- 一个 RTC（Versal 内置）
- 两个 GEM（Cadence MACB 以太网 MAC）
- 八个 ADMA（Xilinx zDMA）通道
- 两个 SD 控制器
- OCM（256KB 片上内存）
- XRAM（4MB 片上加速器 RAM）
- DDR 内存
- BBRAM（36 字节电池备份 RAM）
- eFUSE（3072 字节一次性现场可编程位阵列）
- 两个 CANFD
- USB 控制器
- OSPI 控制器
- TRNG 控制器

Versal Gen 2
""""""""""""

已实现的 CPU 核心：

- 八个 ACPU（Arm Cortex-A78AE），以及相应 GICv3 和 ITS
- 十个 RCPU（Arm Cortex-R52），以及相应 GICv3（每集群一个）

已实现的设备：

- 两个 UART（Arm PL011）
- 一个 RTC（Versal 内置）
- 三个 GEM（Cadence MACB 以太网 MAC）
- 八个 ADMA（Xilinx zDMA）通道
- 两个 SD 控制器
- OCM（256KB 片上内存）
- DDR 内存
- BBRAM（36 字节电池备份 RAM）
- 两个 CANFD
- 两个 USB 控制器
- OSPI 控制器
- TRNG 控制器

QEMU 尚未模拟其他设备，包括 PL 和 AI 引擎。

硬件与 QEMU 模型之间的其他差异：

- QEMU 允许通过 ``-m`` 参数指定 DDR 内存大小。如果命令行提供 DTB，QEMU 会编辑它，加入描述 Versal DDR 内存范围的适当条目。

- QEMU 提供八个 virtio-mmio virtio 传输。它们使用保留内存区域和 IRQ 引脚，避免与真实 SoC 外设冲突。

运行
"""""""

用户提供要加载的操作系统时，应使用 ``-kernel`` 命令行选项。

可通过 ``-device loader`` 选项加载固件或引导加载程序。

加载操作系统时，QEMU 生成 DTB，并选择适当地址加载。该 DTB 通过寄存器 x0 传递给内核。

未指定 ``-kernel`` 时，生成 DTB 并放在 0x1000，供引导加载程序或固件获取。要转储并查看生成的 DTB，可使用 ``dumpdtb`` 机器选项：

.. code-block:: bash

  $ qemu-system-aarch64 -M amd-versal-virt,dumpdtb=example.dtb -m 2G



如果用户希望提供自己的 DTB，可使用 ``-dtb`` 选项。在传递给内核或固件之前，这些 DTB 的内存节点会被修改，以匹配 QEMU 选择的 ram_size 选项。

加载操作系统时，启用 QEMU 的 PSCI 实现，以 SMC 作为 PSCI 调用通道。未指定 ``-kernel`` 时，假定用户提供处理 PSCI 的 EL3 固件。

以下给出一些示例。

直接启动通用 ARM64 上游 Linux 内核：

.. code-block:: bash

  $ qemu-system-aarch64 -M amd-versal-virt -m 2G \
      -serial mon:stdio -display none \
      -kernel arch/arm64/boot/Image \
      -nic user -nic user \
      -device virtio-rng-device,bus=virtio-mmio-bus.0 \
      -drive if=none,index=0,file=hd0.qcow2,id=hd0,snapshot \
      -drive file=qemu_sd.qcow2,if=sd,index=0,snapshot \
      -device virtio-blk-device,drive=hd0 -append root=/dev/vda


直接启动 PetaLinux 2019.2：

.. code-block:: bash

  $ qemu-system-aarch64  -M amd-versal-virt -m 2G \
      -serial mon:stdio -display none \
      -kernel petalinux-v2019.2/Image \
      -append "rdinit=/sbin/init console=ttyAMA0,115200n8 earlycon=pl011,mmio,0xFF000000,115200n8" \
      -net nic,model=cadence_gem,netdev=net0 -netdev user,id=net0 \
      -device virtio-rng-device,bus=virtio-mmio-bus.0,rng=rng0 \
      -object rng-random,filename=/dev/urandom,id=rng0


通过 Arm Trusted Firmware（采用 2018.3，因为 2019.2 版 ATF 会尝试配置尚未模拟的 CCI）和 U-Boot 启动 PetaLinux 2019.2：

.. code-block:: bash

  $ qemu-system-aarch64 -M amd-versal-virt -m 2G \
      -serial stdio -display none \
      -device loader,file=petalinux-v2018.3/bl31.elf,cpu-num=0 \
      -device loader,file=petalinux-v2019.2/u-boot.elf \
      -device loader,addr=0x20000000,file=petalinux-v2019.2/Image \
      -nic user -nic user \
      -device virtio-rng-device,bus=virtio-mmio-bus.0,rng=rng0 \
      -object rng-random,filename=/dev/urandom,id=rng0


在 U-Boot 提示符下运行：

.. code-block:: bash

  Versal>
  fdt addr $fdtcontroladdr
  fdt move $fdtcontroladdr 0x40000000
  fdt set /timer clock-frequency <0x3dfd240>
  setenv bootargs "rdinit=/sbin/init maxcpus=1 console=ttyAMA0,115200n8 earlycon=pl011,mmio,0xFF000000,115200n8"
  booti 20000000 - 40000000
  fdt addr $fdtcontroladdr


通过 U-Boot 在 Xen 上将 Linux 作为 DOM0 启动：

.. code-block:: bash

  $ qemu-system-aarch64 -M amd-versal-virt -m 4G \
      -serial stdio -display none \
      -device loader,file=petalinux-v2019.2/u-boot.elf,cpu-num=0 \
      -device loader,addr=0x30000000,file=linux/2018-04-24/xen \
      -device loader,addr=0x40000000,file=petalinux-v2019.2/Image \
      -nic user -nic user \
      -device virtio-rng-device,bus=virtio-mmio-bus.0,rng=rng0 \
      -object rng-random,filename=/dev/urandom,id=rng0


在 U-Boot 提示符下运行：

.. code-block:: bash

  Versal>
  fdt addr $fdtcontroladdr
  fdt move $fdtcontroladdr 0x20000000
  fdt set /timer clock-frequency <0x3dfd240>
  fdt set /chosen xen,xen-bootargs "console=dtuart dtuart=/uart@ff000000 dom0_mem=640M bootscrub=0 maxcpus=1 timer_slop=0"
  fdt set /chosen xen,dom0-bootargs "rdinit=/sbin/init clk_ignore_unused console=hvc0 maxcpus=1"
  fdt mknode /chosen dom0
  fdt set /chosen/dom0 compatible "xen,multiboot-module"
  fdt set /chosen/dom0 reg <0x00000000 0x40000000 0x0 0x03100000>
  booti 30000000 - 20000000


通过 Arm Trusted Firmware 和 U-Boot 在 Xen 上将 Linux 作为 Dom0 启动：

.. code-block:: bash

  $ qemu-system-aarch64 -M amd-versal-virt -m 4G \
      -serial stdio -display none \
      -device loader,file=petalinux-v2018.3/bl31.elf,cpu-num=0 \
      -device loader,file=petalinux-v2019.2/u-boot.elf \
      -device loader,addr=0x30000000,file=linux/2018-04-24/xen \
      -device loader,addr=0x40000000,file=petalinux-v2019.2/Image \
      -nic user -nic user \
      -device virtio-rng-device,bus=virtio-mmio-bus.0,rng=rng0 \
      -object rng-random,filename=/dev/urandom,id=rng0


在 U-Boot 提示符下运行：

.. code-block:: bash

  Versal>
  fdt addr $fdtcontroladdr
  fdt move $fdtcontroladdr 0x20000000
  fdt set /timer clock-frequency <0x3dfd240>
  fdt set /chosen xen,xen-bootargs "console=dtuart dtuart=/uart@ff000000 dom0_mem=640M bootscrub=0 maxcpus=1 timer_slop=0"
  fdt set /chosen xen,dom0-bootargs "rdinit=/sbin/init clk_ignore_unused console=hvc0 maxcpus=1"
  fdt mknode /chosen dom0
  fdt set /chosen/dom0 compatible "xen,multiboot-module"
  fdt set /chosen/dom0 reg <0x00000000 0x40000000 0x0 0x03100000>
  booti 30000000 - 20000000


可以通过机器模型选项 ``ospi-flash`` 更改模拟的 OSPI 闪存型号。

BBRAM 文件后端
""""""""""""""""""

BBRAM 可使用可选文件后端，必须是大小至少为 36 字节且可定位的二进制文件。全为二进制 0 的文件表示“空白”。

为 BBRAM 添加文件后端：

.. code-block:: bash

  -drive if=pflash,index=0,file=versal-bbram.bin,format=raw


要使用不同于默认值 0 的索引 N，添加：

.. code-block:: bash

  -global driver=xlnx.bbram-ctrl,property=drive-index,value=N


eFUSE 文件后端
""""""""""""""""""

.. note::
   Versal Gen 2 QEMU 模型尚未实现 eFUSE 设备。

eFUSE 可使用可选文件后端，必须是大小至少为 3072 字节且可定位的二进制文件。全为二进制 0 的文件表示“空白”。

为 eFUSE 添加文件后端：

.. code-block:: bash

  -drive if=pflash,index=1,file=versal-efuse.bin,format=raw


要使用不同于默认值 1 的索引 N，添加：

.. code-block:: bash

  -global xlnx-efuse.drive-index=N


.. warning::
  在实际物理 Versal 中，BBRAM 和 eFUSE 包含敏感数据。QEMU 设备模型在将数据保存在模型内存或写入文件后端时，**不会** 对数据进行加密或混淆。

  因此，应谨慎使用文件后端，强烈建议采用 'format=luks'，尽管使用更复杂。

  更好的做法是在此 AMD Versal Virt 板上运行客户机镜像时，不使用真实产品数据。

在 Versal Virt 上使用 CANFD
""""""""""""""""""""""""""""

Versal CANFD 控制器基于 SocketCAN 和 QEMU CAN 总线实现开发。每个 CAN 模块的总线连接和 SocketCAN 连接均可通过命令行设置。

将 CANFD0 和 CANFD1 连接到同一总线：

.. code-block:: bash

    -object can-bus,id=canbus -machine canbus0=canbus -machine canbus1=canbus


将 CANFD0 和 CANFD1 连接到独立总线：

.. code-block:: bash

    -object can-bus,id=canbus0 -object can-bus,id=canbus1 \
    -machine canbus0=canbus0 -machine canbus1=canbus1


SocketCAN 接口可连接到主机上的物理或虚拟 CAN 接口。关于 Linux CAN 接口，请参阅 docs/system/devices/can.rst。

将 CANFD0 和 CANFD1 连接到主机 CAN 接口 can0：

.. code-block:: bash

    -object can-bus,id=canbus -machine canbus0=canbus -machine canbus1=canbus
    -object can-host-socketcan,id=canhost0,if=can0,canbus=canbus


.. note::
   Versal Gen 2 有四个 CAN 控制器，可在命令行指定 ``canbus0`` 至 ``canbus3``。
