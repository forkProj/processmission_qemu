================================================================
AMD SEV 和 SEV-ES 的 QEMU／客户机固件接口
================================================================

概述
========================================

客户机固件镜像 OVMF 可能包含 QEMU 在客户机启动前使用的配置条目。
它们列在固件中已知位置、以 GUID 标识的表中。
QEMU 将固件加载到内存时解析该表，需要时读取各条目值。

表结构本身并不专用于 SEV，但目前所有条目都与 SEV、SEV-ES 功能有关。


QEMU 中的表解析
----------------------------------------

从表尾开始解析：先验证 0xffffffd0 处是否存在表尾 GUID
96b582de-1fb2-45f7-baea-a366c55a082d。
存在时，0xffffffce 处的两个字节表示整个表的长度。

随后向前反向扫描，寻找指定条目 GUID。

相关 QEMU 文件：

- ``hw/i386/pc_sysfw_ovmf.c``

构造此结构的 edk2 代码位于 `OVMF Reset Vector file`_。


表内存布局
----------------------------------------

.. list-table::
   :header-rows: 1

   * - GPA
     - 长度
     - 描述
   * - 0xffffff80
     - 4
     - 零填充
   * - 0xffffff84
     - 4
     - SEV 哈希表基址
   * - 0xffffff88
     - 4
     - SEV 哈希表大小（0x400）
   * - 0xffffff8c
     - 2
     - SEV 哈希表条目长度（0x1a）
   * - 0xffffff8e
     - 16
     - SEV 哈希表 GUID：7255371f-3a3b-4b04-927b-1da6efa8d454
   * - 0xffffff9e
     - 4
     - SEV 秘密数据块基址
   * - 0xffffffa2
     - 4
     - SEV 秘密数据块大小（0xc00）
   * - 0xffffffa6
     - 2
     - SEV 秘密数据块条目长度（0x1a）
   * - 0xffffffa8
     - 16
     - SEV 秘密数据块 GUID：4c2eb361-7d9b-4cc3-8081-127c90d3d294
   * - 0xffffffb8
     - 4
     - SEV-ES AP 复位 RIP
   * - 0xffffffbc
     - 2
     - SEV-ES 复位块条目长度（0x16）
   * - 0xffffffbe
     - 16
     - SEV-ES 复位块条目 GUID：00f771de-1a7e-4fcb-890e-68c77e2fb44e
   * - 0xffffffce
     - 2
     - 整个表的长度，含表尾 GUID 和长度字段（0x72）
   * - 0xffffffd0
     - 16
     - OVMF GUID 表的表尾 GUID：96b582de-1fb2-45f7-baea-a366c55a082d
   * - 0xffffffe0
     - 8
     - 应用处理器入口代码
   * - 0xffffffe8
     - 8
     - "\0\0\0\0VTF\0"
   * - 0xfffffff0
     - 16
     - 复位向量代码


表条目说明
========================================

SEV-ES 复位块
----------------------------------------

条目 GUID：00f771de-1a7e-4fcb-890e-68c77e2fb44e

SEV-ES 下 AP 首次启动时，复位 RIP 必须设为此条目定义的 RAM 区域。
条目格式：

* IP 值 [0:15]
* CS 段基址 [31:16]

虚拟机管理器读取 CS 段基址和 IP。
其中的 CS 基址值表示实际段基址的高 16 位，因此必须左移 16 位，
形成写入 CS 段寄存器的完整基址；EIP 则设为直接读取的 IP 值。


SEV 秘密数据块
----------------------------------------

条目 GUID：4c2eb361-7d9b-4cc3-8081-127c90d3d294

描述虚拟机管理器通过 SEV_LAUNCH_SECRET 注入客户机所有者秘密数据的客户机 RAM 区域。


SEV 哈希表
----------------------------------------

条目 GUID：7255371f-3a3b-4b04-927b-1da6efa8d454

描述虚拟机管理器应安装哈希表的客户机 RAM 区域。
表记录特定固件配置设备文件的哈希，否则这些文件将未经检查地传入。
目前用于内核、initrd 和命令行值，将来可能增加其他内容。


.. _OVMF Reset Vector file:
   https://github.com/tianocore/edk2/blob/master/OvmfPkg/ResetVector/Ia16/ResetVectorVtf0.asm
