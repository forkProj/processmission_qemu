BOSC 香山昆明湖 FPGA 原型平台 (``xiangshan-kunminghu``)
==========================================================================
该机器兼容香山 FPGA 原型平台。

香山是开源高性能 RISC-V 处理器项目，第三代名为昆明湖，是 64 位
RV64GCBSUHV 内核。更多信息见 GitHub 仓库：
https://github.com/OpenXiangShan/XiangShan

支持的设备
-----------------
支持以下设备：

* 最多 16 个香山昆明湖内核
* 内核本地中断器（CLINT）
* 传入 MSI 控制器（IMSIC）
* 高级平台级中断控制器（APLIC）
* 1 个 UART

启动选项
------------
使用标准 -bios 加载启动镜像。需将固件、内核和设备树（FDT）编译链接为
一个二进制文件，例如 fw_payload.bin。

运行
-------
启动命令示例如下：

.. code-block:: bash

   $ qemu-system-riscv64 -machine xiangshan-kunminghu \
      -smp 16 -m 16G \
      -bios path/to/opensbi/platform/generic/firmware/fw_payload.bin \
      -nographic
