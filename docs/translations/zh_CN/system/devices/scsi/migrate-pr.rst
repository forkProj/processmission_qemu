..
    SPDX-License-Identifier: GPL-2.0-or-later

.. _scsi_migrate_pr:

SCSI 持久预留热迁移
==========================================

本文介绍如何热迁移 SCSI 持久预留。

scsi-block 在 migrate-pr=on 时迁移预留。QEMU 11.0 起的版本化机器默认
启用，旧机器默认禁用，需显式使用 --device scsi-block,migrate-pr=on,...。

启用后，QEMU 监听 PERSISTENT RESERVATION OUT，跟踪客户机注册的密钥及
取得的预留。信息随客户机迁移，目的端在客户机运行前发送带 PREEMPT 服务
动作的命令，原子地转移预留。

启用迁移时，QEMU 会从 REPORT CAPABILITIES 返回值中屏蔽：

 * Specify Initiator Ports Capable（SIP_C）
 * All Target Ports Capable（ATC_C）

禁用时设备仍可迁移，但预留留在源端。除非有其他机制更新，通常不符合预期。
此时也不屏蔽向客户机报告的能力。

限制
-----------

重启不保留监听到的预留详情，因此客户机软件必须在启动后取得预留，才能
随之迁移。在客户机之外取得的预留也不会随迁移转移。

只跟踪客户机的 OUT 命令，不跟踪其他 SCSI 发起方的修改，因此记录可能过时。

同主机客户机共享一个 SCSI 发起方身份，除非配置 Fibre Channel N_Port ID
Virtualization。因此多个客户机使用同一物理 LUN 可能出现意外行为：从 LUN
看它们是同一个发起方，无法区分。
