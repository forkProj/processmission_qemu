XBZRLE（基于异或的零游程长度编码）
==================================

使用 XBZRLE（Xor Based Zero Run Length Encoding，基于异或的零游程长度编码）可以减少虚拟机停机时间和实时迁移总时间。它尤其适合运行内存写入密集型负载的虚拟机，例如 SAP ERP 系统等大型企业应用的典型负载；一般而言，也适合采用稀疏内存更新模式的任何应用。

该方案发送更新内容的压缩版本，而不是整个发生变化的客户机内存页，从而减少实时迁移的数据传输量。为了计算更新，需要在源端保存之前的内存页。这些页存储在专用缓存（哈希表）中，通过地址访问。缓存越大，所需页面已经位于缓存中的概率就越高。缓存较小会导致较高的未命中率。缓存大小可以在迁移前和迁移过程中修改。

格式
--------

压缩格式对页面的旧内容和当前内容进行异或，其中零表示值没有变化。页面数据增量由零游程和非零游程表示。零游程用其长度（字节数）表示。非零游程用长度（字节数）及新数据表示。游程长度使用 ULEB128 编码（http://en.wikipedia.org/wiki/LEB128）。

有效编码可能不止一种；为了降低计算成本，发送方可以采用较长的编码。

::

    page = zrun nzrun
           | zrun nzrun page

    zrun = length

    nzrun = length byte...

    length = uleb128 编码整数


发送端从缓存（默认大小 64 MB）获取旧页面内容，使用 XBZRLE 对页面更新进行紧凑的增量编码。接收端利用现有页面内容和 XBZRLE 解码出新页面内容。

这项工作最初基于 Benoit、Svard、Tordsson 和 Elmroth 在 VEE 2011 上发表的研究结果《Evaluation of Delta Compression Techniques for Efficient Live Migration of Large Virtual Machines》（高效迁移大型虚拟机的增量压缩技术评估）。此外，还用 XBZRLE 替代 XBRLE，进一步改进了增量编码器。

对于典型负载，XBZRLE 的持续带宽达到 2–2.5 GB/s，非常适合实时迁移所需的在线实时编码。

示例：

旧缓冲区：

.. code:: batch

    1001 zeros
    05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 68 00 00 6b 00 6d
    3074 zeros


新缓冲区：

.. code:: batch

    1001 zeros
    01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 68 00 00 67 00 69
    3074 zeros


编码后的缓冲区：

.. code:: batch

    encoded length 24
    e9 07 0f 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 03 01 67 01 01 69


缓存更新策略
------------

将热点页面保留在缓存中能有效减少未命中。XBZRLE 使用计数器表示每个页面的年龄。每次同步 RAM 脏位图后，计数器递增。检测到缓存冲突时，XBZRLE 只会淘汰缓存中年龄超过阈值的页面。

用法
--------

1. 确认目标 QEMU 版本能够解码新格式::

    (qemu) info migrate_capabilities
    xbzrle: off
    ...


2. 在源端和目标端都启用 xbzrle::

    (qemu) migrate_set_capability xbzrle on


3. 设置 XBZRLE 缓存大小。单位为 MB，应为 2 的幂。默认大小是 64 MB（只在源端设置）::

    (qemu) migrate_set_parameter xbzrle-cache-size 256m


4. 启动向外迁移::

    (qemu) migrate -d tcp:destination.host:4444
    (qemu) info migrate
    capabilities: xbzrle: on
    Migration status: active
    transferred ram: A kbytes
    remaining ram: B kbytes
    total ram: C kbytes
    total time: D milliseconds
    duplicate: E pages
    normal: F pages
    normal bytes: G kbytes
    cache size: H bytes
    xbzrle transferred: I kbytes
    xbzrle pages: J pages
    xbzrle cache miss: K pages
    xbzrle cache miss rate: L
    xbzrle encoding rate: M
    xbzrle overflow: N


xbzrle cache miss：截至目前的缓存未命中次数。较高的未命中率表明缓存设置过小。

xbzrle overflow：无法压缩增量时产生的解码溢出次数。如果页面变化过大或存在大量短小变化，就可能发生这种情况，例如每隔一个字节修改一次（修改半个页面）。

测试：测试表明，启用 XBZRLE 后实时迁移在 110 秒内完成，而不启用时无法完成迁移。

一个简单的合成内存读写负载生成器：

.. code-block:: c

    #include <stdlib.h>
    #include <stdio.h>
    int main()
    {
        char *buf = (char *) calloc(4096, 4096);
        while (1) {
            int i;
            for (i = 0; i < 4096 * 4; i++) {
                buf[i * 4096 / 4]++;
            }
            printf(".");
        }
    }
