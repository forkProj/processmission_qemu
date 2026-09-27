.. SPDX-License-Identifier: GPL-2.0-or-later

==============
eMMC 模拟
==============

除 SD 卡外，QEMU 还提供嵌入式开发板常见的 eMMC 模型，同样通过 SDHCI
控制器连接机器。

创建 eMMC 镜像
==================

现代 eMMC 有四部分：两个启动分区、一个重放保护内存块（RPMB）和用户数据区。
镜像必须严格按此顺序拼接；若配置大小为零，启动分区或 RPMB 可以省略。

规范要求两个启动分区大小相同，启动和 RPMB 大小为 128 KB 倍数。每个启动
分区最大 32640 KB，RPMB 最大 16384K。

用户区不超过 2 GB 时，大小必须为 2 的幂；从 2 GB 起必须为 512 字节倍数。

QEMU 创建设备前强制检查，镜像必须严格遵循。scripts/mkemmc.sh 可创建合规
镜像，支持预填内容。例如由固件和操作系统镜像生成，附带空的 2 MB RPMB：

.. code-block:: console

    scripts/mkemmc.sh -b firmware.img -r /dev/zero:2M os.img emmc.img


脚本将大小向上对齐到合法值，RPMB 和第二启动分区填零。

添加 eMMC 设备
===================

机器模型可能自动创建 eMMC，例如 Aspeed；也可搭配 PCI SDHCI 用户创建。
假定机器没有其他 SDHCI，固件需要 1 MB 启动分区，上例镜像配置如下：

.. code-block:: console

    -drive file=emmc.img,if=none,format=raw,id=emmc-img
    -device sdhci-pci
    -device emmc,drive=emmc-img,boot-partition-size=1048576,rpmb-partition-size=2097152


RPMB 认证密钥
=======================

私有共享密钥用于验证主机对 RPMB 的请求。真实 eMMC 持久保存，设置后不能
重写。QEMU 模拟编程过程，但重启不保留状态。要模拟已设置密钥，可通过
属性 auth-key 创建：

.. code-block:: console

    -device emmc,[...],auth-key=D3EB3EC36E334C9F988CE2C0B85954610D2BCF8664844DF2AB56E6C61BB701E4


这使用 OP-TEE 的公开测试密钥。若设备已由机器创建：

.. code-block:: console

    -global emmc.auth-key=D3EB3EC36E334C9F988CE2C0B85954610D2BCF8664844DF2AB56E6C61BB701E4


密钥始终为 32 字节，必须十六进制编码，必要时左侧补零。
