.. _ebpf-rss:

===========================
virtio-net 的 eBPF RSS 支持
===========================

RSS（接收侧扩展）通过计算数据包哈希，将网络包分配到客户机 virtqueue。
通常每个队列由特定客户机 CPU 核心处理。

QEMU 目前有两种实现：

- in-qemu RSS：QEMU 接收网络包时工作，即 vhost=off。
- eBPF RSS：也可在 vhost=on 时工作。

eBPF 支持（CONFIG_EBPF）由 configure 脚本启用。
使用 ./configure --enable-bpf 启用 eBPF RSS。

若未为内核 TUN 模块设置分流 BPF，TUN 根据已发送包的对称哈希建立查找表，
自动选择接收 virtqueue。
若已设置，则由 BPF 计算包头哈希，返回应接收该包的队列号。

简化公式：

.. code:: C

    queue_index = indirection_table[hash(<packet data>)%<indirection_table size>]



并非所有数据包都能或都应该计算哈希。

注意：当前 eBPF RSS 不支持哈希报告。

vhost-net、virtio-net 和 tap 的组合决定所用实现：

- 使用 eBPF：

        tap,vhost=off & virtio-net-pci,rss=on,hash=off

- 使用 eBPF：

        tap,vhost=on & virtio-net-pci,rss=on,hash=off

- 使用 in-qemu RSS：

        tap,vhost=off & virtio-net-pci,rss=on,hash=on

- 使用 eBPF，但不向客户机报告哈希填充功能：

        tap,vhost=on & virtio-net-pci,rss=on,hash=on

未设置 CONFIG_EBPF 时，只支持 in-qemu RSS。
eBPF 程序加载失败或设置到 TUN 失败时，也会回退到 in-qemu RSS。

RSS eBPF 程序
-------------

RSS 程序位于 bpftool 生成的 ebpf/rss.bpf.skeleton.h，因此属于 QEMU 二进制文件的一部分。
原始 eBPF 程序由 Clang 编译，源码位于 tools/ebpf/rss.bpf.c。
重新编译，即重新生成 ebpf/rss.bpf.skeleton.h，需要：

        llvm、clang、内核源码树、bpftool
        调整 Makefile.ebpf，使其指向正确的内核源码树

        $ cd tools/ebpf
        $ make -f Makefile.ebpf

当前实现使用较新内核中的有限循环和后向跳转指令，整体需要内核 5.8+。

eBPF RSS 实现
-------------

eBPF RSS 加载功能位于 ebpf/ebpf_rss.c 和 ebpf/ebpf_rss.h。

``struct EBPFRSSContext`` 保存上下文及四个文件描述符：

- ctx：libbpf 上下文指针。
- program_fd：eBPF RSS 程序文件描述符。
- map_configuration：configuration 映射描述符，包含一个 EBPFRSSConfig，决定程序行为。
- map_toeplitz_key：Toeplitz key 映射描述符，包含一个供哈希算法使用的 40 字节密钥。
- map_indirections_table：包含 128 个队列索引的映射。

``struct EBPFRSSConfig`` 的字段：

- redirect：是否计算哈希的布尔值，false 时使用 default_queue。
- populate_hash：目前未使用，因为 eBPF RSS 不支持哈希报告。
- hash_types：哈希类型位掩码，见 VIRTIO_NET_RSS_HASH_TYPE_*。
  不应计算哈希的包使用 default_queue。
- indirections_len：间接表长度，最大 128。
- default_queue：不应计算哈希的数据包使用的队列。
  某些包，例如 ARP，无法计算哈希。

函数：

- ``ebpf_rss_init()``：将 ctx 设为 NULL，表示尚未加载上下文。
- ``ebpf_rss_load()``：创建三个映射并从 rss.bpf.skeleton.h 加载程序，成功返回 true。
  之后可用 program_fd 设置 TAP 分流。
- ``ebpf_rss_set_all()``：设置映射值。
  indirections_table 长度保存在 EBPFRSSConfig；toeplitz_key 是
  VIRTIO_NET_RSS_MAX_KEY_SIZE 大小，即 40 字节的数组。
- ``ebpf_rss_unload()``：关闭全部文件描述符，将 ctx 设为 NULL。

简化流程：

.. code:: C

    struct EBPFRSSConfig config;
    config.redirect = 1;
    config.hash_types = VIRTIO_NET_RSS_HASH_TYPE_UDPv4 | VIRTIO_NET_RSS_HASH_TYPE_TCPv4;
    config.indirections_len = VIRTIO_NET_RSS_MAX_TABLE_LEN;
    config.default_queue = 0;

    uint16_t table[VIRTIO_NET_RSS_MAX_TABLE_LEN] = {...};
    uint8_t key[VIRTIO_NET_RSS_MAX_KEY_SIZE] = {...};

    struct EBPFRSSContext ctx;
    ebpf_rss_init(&ctx);
    ebpf_rss_load(&ctx);
    ebpf_rss_set_all(&ctx, &config, table, key);
    if (net_client->info->set_steering_ebpf != NULL) {
        net_client->info->set_steering_ebpf(net_client, ctx->program_fd);
    }
    ...
    ebpf_unload(&ctx);



NetClientState SetSteeringEBPF()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

当前 Linux TAP NetClientState 支持 set_steering_ebpf()，
参数为 eBPF 程序文件描述符。
