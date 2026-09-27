Shakti C 参考平台（``shakti_c``）
==========================================

Shakti C 参考平台是基于 Arty A7 100T、面向 Shakti SoC 的参考平台。

Shakti SoC 基于 Shakti C-class 处理器核心。Shakti C 是一个 64 位 RV64GCSUN 处理器核心。

关于 Shakti SoC 的更多详情，请参见：
https://gitlab.com/shaktiproject/cores/shakti-soc/-/blob/master/fpga/boards/artya7-100t/c-class/README.rst

关于 Shakti C-class 核心的更多信息，请参见：
https://c-class.readthedocs.io/en/latest/

支持的设备
-----------------

``shakti_c`` 机器支持以下设备：

 * 一个 C-class 核心
 * 核心级中断器（CLINT）
 * 平台级中断控制器（PLIC）
 * 一个 UART

启动选项
------------

``shakti_c`` 机器可使用标准 -bios 功能加载裸机应用或 OpenSBI 并启动。

启动机器
----------------

Shakti SDK
~~~~~~~~~~

可以使用 Shakti SDK 生成裸机 UART 示例应用。

.. code-block:: bash

   $ git clone https://gitlab.com/behindbytes/shakti-sdk.git
   $ cd shakti-sdk
   $ make software PROGRAM=loopback TARGET=artix7_100t


二进制文件将生成在：
  software/examples/uart_applns/loopback/output/loopback.shakti

也可以使用以下命令下载预编译的示例应用。

.. code-block:: bash

   $ wget -c https://gitlab.com/behindbytes/shakti-binaries/-/raw/master/sdk/shakti_sdk_qemu.zip
   $ unzip shakti_sdk_qemu.zip


随后可以使用以下命令运行 UART 示例：

.. code-block:: bash

   $ qemu-system-riscv64 -M shakti_c -nographic \
      -bios path/to/shakti_sdk_qemu/loopback.shakti


OpenSBI
~~~~~~~

也可以运行带测试载荷的 OpenSBI。

.. code-block:: bash

   $ git clone https://github.com/riscv/opensbi.git -b v0.9
   $ cd opensbi
   $ wget -c https://gitlab.com/behindbytes/shakti-binaries/-/raw/master/dts/shakti.dtb
   $ export CROSS_COMPILE=riscv64-unknown-elf-
   $ export FW_FDT_PATH=./shakti.dtb
   $ make PLATFORM=generic


fw_payload.elf 将生成在 build/platform/generic/firmware/fw_payload.elf。使用以下 QEMU 命令启动：

.. code-block:: bash

   $ qemu-system-riscv64 -M shakti_c -nographic \
      -bios path/to/fw_payload.elf
