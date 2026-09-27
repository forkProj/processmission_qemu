MIPS RISC-V 处理器开发板
====================================

MIPS 开发的 RISC-V 处理器支持 Boston-aia 模型，最多 64 个 hart，包含
MIPS CPS、GCR、CPC、AIA plic 和 AIA clint。未指定 ``-bios`` 时可生成
启动代码，也可指定 ``-smp x,cores=y,thread=z``。

运行 Linux 内核
--------------------

例如，在 mips-p8700 上使用 4 核、每核 2 线程，共 8 个 SMP CPU：

.. code-block:: bash

   qemu-system-riscv64 -cpu mips-p8700 \
         -m 2G -M boston-aia \
         -smp 8,cores=4,threads=2 -kernel fw_payload.bin \
         -drive file=rootfs.ext2,format=raw -serial stdio
