Milk-V Duo 板（``milkv-duo``）
================================

``milkv-duo`` 机器模拟采用 Sophgo CV1800B SoC 的 Milk-V Duo 板，面向 CV1800B 平台的早期固件、内核和设备模型开发。

支持的设备
-----------------

``milkv-duo`` 机器支持以下设备：

* 最多两个 T-Head C906 RISC-V hart
* 核心本地中断器（ACLINT）
* 平台级中断控制器（PLIC）
* 兼容 DesignWare 8250 的 UART0
* 兼容 SDHCI 的 SD0 控制器
* CV1800B 时钟控制器

若干 CV1800B MMIO 区域以未实现设备的形式存在，使固件或客户机软件能够在对应设备模型尚不完整时探测预期的地址映射。

启动选项
------------

此机器支持使用 ``-kernel`` 直接加载载荷，也支持使用 ``-bios`` 进入固件。模型不生成设备树，因此直接启动 Linux 时，需要通过 ``-dtb`` 传入外部 CV1800B 或 Milk-V Duo DTB。

直接启动 Linux
~~~~~~~~~~~~~~~~~

使用 ``-kernel`` 加载内核镜像，使用 ``-dtb`` 提供板级设备树，并可选择使用 ``-initrd`` 和 ``-append`` 提供根文件系统和内核命令行：

.. code-block:: bash

   $ qemu-system-riscv64 -machine milkv-duo \
      -smp 1 -m 64M \
      -kernel /path/to/Image \
      -dtb /path/to/milkv-duo.dtb \
      -initrd /path/to/initramfs.cpio.gz \
      -append "console=ttyS0" \
      -serial mon:stdio \
      -display none


固件启动
~~~~~~~~~~~~~

使用 ``-bios`` 进入 CV1800B 固件镜像。固件镜像加载到 DRAM 起始位置，并从模拟的启动 ROM 复位向量进入：

.. code-block:: bash

   $ qemu-system-riscv64 -machine milkv-duo \
      -smp 1 -m 64M \
      -bios /path/to/firmware.elf \
      -serial mon:stdio \
      -display none


已知限制
-----------------

当前模型重点实现早期软件启动所需的 CPU、中断控制器、UART、SDHCI 控制器和时钟控制器路径。其他 CV1800B 外设块以未实现的 MMIO 区域表示。

运行测试
-------------

Milk-V Duo qtest 覆盖 UART 组件 ID 寄存器以及 CV1800B 时钟控制器的基本读写行为：

.. code-block:: bash

   $ QTEST_QEMU_BINARY=./build/qemu-system-riscv64 \
      ./build/tests/qtest/milkv-duo-test --tap -k
