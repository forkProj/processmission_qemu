QEMU NBD 协议支持
================================================

QEMU 支持 NBD，包含内部客户端 ``block/nbd.c``、内部服务器 ``blockdev-nbd.c``，
以及独立服务器工具 ``qemu-nbd.c``。公共代码位于 ``nbd/*``。

协议规范位于：
https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md

以下介绍 QEMU 实现的一些特有属性。

元数据命名空间
----------------------------------------

QEMU 支持规范定义的 base:allocation 元数据上下文，并额外定义 qemu 命名空间。

``qemu`` 命名空间
----------------------------------------

目前包含两种元数据上下文。第一种在导出磁盘内容的同时暴露脏位图内容，命名形式为::

    qemu:dirty-bitmap:<dirty-bitmap-export-name>

每个脏位图上下文在 NBD_CMD_BLOCK_STATUS 回复中，为区段定义一个标志：

位 0：
  NBD_STATE_DIRTY，区段为脏时设置。

第二种暴露镜像各区段的数据来源，使用单一上下文::

    qemu:allocation-depth

分配深度上下文的完整 32 位值表示精简配置后备链中哪一层提供数据：
0 未分配，1 活动层，2 第一后备层，依此类推。

NBD_OPT_LIST_META_CONTEXT 除接受具体的 qemu:allocation-depth 和
qemu:dirty-bitmap:<dirty-bitmap-export-name> 外，还支持：

``qemu:``
  返回命名空间内全部可用元数据上下文。
``qemu:dirty-bitmap:``
  返回全部可用脏位图上下文。

各版本功能
----------------------------------------

以下列出各功能首次实现的 QEMU 版本，包括服务器提供和客户端使用两方面，
便于规划跨版本互操作。部分功能的初始版本可能需要相应稳定分支的额外补丁才能修复错误。

2.6
  基于 TLS X.509 证书的 NBD_OPT_STARTTLS。
2.8
  NBD_CMD_WRITE_ZEROES。
2.10
  NBD_OPT_GO、NBD_INFO_BLOCK。
2.11
  NBD_OPT_STRUCTURED_REPLY。
2.12
  base:allocation 的 NBD_CMD_BLOCK_STATUS。
3.0
  使用 TLS 预共享密钥（PSK）的 NBD_OPT_STARTTLS；
  qemu:dirty-bitmap: 的 NBD_CMD_BLOCK_STATUS，以及 NBD_CMD_CACHE。
4.2
  可共享只读导出的 NBD_FLAG_CAN_MULTI_CONN，以及 NBD_CMD_FLAG_FAST_ZERO。
5.2
  qemu:allocation-depth 的 NBD_CMD_BLOCK_STATUS。
7.1
  可共享可写导出的 NBD_FLAG_CAN_MULTI_CONN。
8.2
  NBD_OPT_EXTENDED_HEADERS、NBD_FLAG_BLOCK_STATUS_PAYLOAD。
