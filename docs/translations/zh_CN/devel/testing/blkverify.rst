使用 ``blkverify`` 测试块驱动正确性
===================================

简介
--------

本文介绍如何使用 blkverify 协议测试块驱动是否正确工作。

使用真实客户机测试和调试块驱动很困难。
客户机进程可能因可执行文件中读到损坏扇区而崩溃，
也可能由客户机程序报告含糊错误。这些现象很难追溯到块驱动中的缺陷。

blkverify 在首次读到错误数据时就在 QEMU 内捕获损坏，并报告对应磁盘扇区。

工作方式
--------

blkverify 有两个子块设备：test 和 raw。
读写操作同时镜像到两者，因此其状态应始终同步。

raw 设备是一个平坦文件形式的原始镜像，初始内容与 test 完全相同。
假定 raw 正确处理读写且不破坏数据，用作与 test 对比的参考。

镜像读取完成后，blkverify 比较数据，不一致时即报错，
因此能够捕获第一次读到损坏数据的情况。

示例
--------

假设 raw.img 的第一个扇区全部为 0xcd::

    $ ./qemu-io -c 'read -v 0 512' raw.img
    00000000:  cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd  ................
    00000010:  cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd  ................
    [...]
    000001e0:  cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd  ................
    000001f0:  cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd cd  ................
    read 512/512 bytes at offset 0
    512.000000 bytes, 1 ops; 0.0000 sec (97.656 MiB/sec and 200000.0000 ops/sec)

test.img 则已损坏，第一个扇区被错误清零::

    $ ./qemu-io -c 'read -v 0 512' test.img
    00000000:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    00000010:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    [...]
    000001e0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    000001f0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    read 512/512 bytes at offset 0
    512.000000 bytes, 1 ops; 0.0000 sec (81.380 MiB/sec and 166666.6667 ops/sec)

blkverify 会捕获此错误::

    $ ./qemu-io -c 'read 0 512' blkverify:a.img:b.img
    blkverify: read sector_num=0 nb_sectors=4 contents mismatch in sector 0

更实际的场景是验证客户机操作系统安装过程::

    $ ./qemu-img create raw.img 16G
    $ ./qemu-img create -f qcow2 test.qcow2 16G
    $ ./qemu-system-x86_64 -cdrom debian.iso \
          -drive file=blkverify:raw.img:test.qcow2

如果 blkverify 发现损坏而中止安装，使用 qemu-io 查看镜像中相应扇区的内容。
