==============
NVMe 模拟
==============

QEMU 通过 nvme、nvme-ns 和 nvme-subsys 设备提供 NVMe 模拟。

详细信息见以下各节：

  * `添加 NVMe 设备`_、`额外命名空间`_ 和 `NVM 子系统`_。
  * `可选功能`_，包括 `控制器内存缓冲区`_、`简单复制`_、`分区命名空间`_、
    `元数据`_ 和 `端到端数据保护`_。

添加 NVMe 设备
===================

控制器模拟
--------------------

控制器实现 NVM Express 1.4。全部强制功能均已实现，但有以下例外和限制：

  * 断电重启后 SMART/Health 日志计数归零。
  * 不支持中断合并，默认禁用。

在 PCI 总线上添加控制器的最简单方式：

.. code-block:: console

    -drive file=nvm.img,if=none,id=nvm
    -device nvme,serial=deadbeef,drive=nvm


下面列出部分通用参数，完整列表见 -device nvme,help。

``max_ioqpairs=UINT32`` （默认 64）
  最大 I/O 队列对数，替代已弃用的 num_queues。

``msix_qsize=UINT16`` （默认 65）
  支持的 MSI-X 向量数。

``mdts=UINT8`` （默认 7）
  最大数据传输大小。

``use-intel-id`` （默认 off）
  自 5.2 起使用分配给 QEMU 的 Red Hat PCI 设备/厂商 ID。设为 on 可恢复
  之前使用的未分配 Intel ID。

``ocp`` （默认 off）
  Open Compute Project 的数据中心 NVMe SSD 规范在 NVMe 之上增加了数据
  中心专用命令和行为。on 使 SMART/Health 扩展日志等 OCP 功能可用，
  这里模拟该日志页的版本 5。

``model`` （默认 QEMU NVMe Ctrl）
  覆盖报告的型号，以更接近某种设备，最长 40 字符。

``firmware-version`` （默认当前 QEMU 版本）
  覆盖报告的固件版本，以更接近某种设备，最长 8 字符。

额外命名空间
---------------------

最简方式只有 NSID 1 的单个命名空间。多命名空间和其他功能需要 nvme-ns。

.. code-block:: console

   -device nvme,id=nvme-ctrl-0,serial=deadbeef
   -drive file=nvm-1.img,if=none,id=nvm-1
   -device nvme-ns,drive=nvm-1
   -drive file=nvm-2.img,if=none,id=nvm-2
   -device nvme-ns,drive=nvm-2


nvme-ns 连接到最近定义的 nvme 设备创建的 nvme-bus，ID 从 1 自动分配。

可用参数：

``nsid`` （默认 0）
  显式指定命名空间 ID。

``uuid`` （默认自动生成）
  设置 UUID，在标识描述符列表中报告为 Namespace UUID。

``nguid``
  设置 NGUID，报告为 Namespace Globally Unique Identifier。必须是恰好
  16 字节的十六进制字符串，或 auto 随机生成。可用连字符分组；不指定则全零。

``eui64``
  设置 EUI-64，报告为 IEEE Extended Unique Identifier。机器类型 6.1 起
  未指定时使用非零默认值，更早机器默认 0。

``bus``
  多个控制器时，可指定连接到哪个 nvme，通过其 id 标识。

NVM 子系统
--------------

控制器关联 nvme-subsys 后，可启用共享命名空间和多路径 I/O 等功能。

.. code-block:: console

   -device nvme-subsys,id=nvme-subsys-0,nqn=subsys0
   -device nvme,serial=deadbeef,subsys=nvme-subsys-0
   -device nvme,serial=deadbeef,subsys=nvme-subsys-0


上例创建含两个控制器的子系统，并使 nvme-ns 支持：

``shared`` （自 6.2 起默认 on）
  命名空间连接到子系统全部控制器，包括后续热插入的控制器。off 表示私有，
  同时只能连接一个控制器。

``detached`` （默认 off）
  on 表示命名空间在子系统中可用，但初始不连接任何控制器。共享命名空间
  若开启此选项，也不会自动连接。

因此添加：

.. code-block:: console

   -drive file=nvm-1.img,if=none,id=nvm-1
   -device nvme-ns,drive=nvm-1,nsid=1
   -drive file=nvm-2.img,if=none,id=nvm-2
   -device nvme-ns,drive=nvm-2,nsid=3,shared=off,detached=on


NSID 1 为共享且初始连接两控制器。NSID 3 因 shared=off 为私有，一次只能
连接一个；由于 detached=on，初始不连接，也不自动连接热插入控制器。

可选功能
=================

控制器内存缓冲区
------------------------

相关 nvme 参数：

``cmb_size_mb=UINT32`` （默认 0）
  在 BAR 2 偏移 0 添加指定大小的控制器内存缓冲区。

``legacy-cmb`` （默认 off）
  默认采用 1.4 方案，CMB 初始禁用，必须由主机显式启用。on 则采用 1.3 行为。

简单复制
-----------

支持 TP 4065 Simple Copy。nvme-ns 可通过以下参数限制复制命令：

``mssrl=UINT16`` （默认 128）
  最大单个来源范围长度 MSSRL，即每个来源范围的最大逻辑块数。

``mcl=UINT32`` （默认 128）
  最大复制长度 MCL，即一次命令全部来源范围的逻辑块总数上限。

``msrc=UINT8`` （默认 127）
  最大来源范围数 MSRC，这是从零计数的值。

分区命名空间
----------------

可按 TP 4053 Zoned Namespaces 将 nvme-ns 设置为 zoned=on。

其他参数包括：

``zoned.zone_size=SIZE`` （默认 128MiB）
  区域大小 ZSZE。

``zoned.zone_capacity=SIZE`` （默认 0）
  区域容量 ZCAP，0 表示等于区域大小。

``zoned.descr_ext_size=UINT32`` （默认 0）
  区域描述符扩展大小 ZDES，必须为 64 字节倍数。

``zoned.cross_read=BOOL`` （默认 off）
  on 允许跨区域边界读取。

``zoned.max_active=UINT32`` （默认 0）
  最大活动资源数 MAR，0 允许所有区域活动。

``zoned.max_open=UINT32`` （默认 0）
  最大打开资源数 MOR，0 允许全部打开。若设置 max_active，此值不得超过它。

``zoned.zasl=UINT8`` （默认 0）
  Zone Append 最大传输大小，与 mdts 一样以最小内存页 CAP.MPSMIN 为单位，
  按 2^n 指定。0 表示继承 mdts。

灵活数据放置
-----------------------

在子系统设置 fdp=on，可支持 TP4146 Flexible Data Placement::

    -device nvme-subsys,id=nvme-subsys-0,nqn=subsys0,fdp=on,fdp.nruh=16


子系统模拟单个 Endurance Group，并在其中支持 FDP。与规范略有不同：
只要子系统配置 FDP，控制器就始终启用 FDP Mode。

启用后可设置：

``fdp.nrg`` （默认 1）
  回收组数量。

``fdp.nruh`` （默认 0）
  回收单元句柄数量，必需且必须非零。

``fdp.runs`` （默认 96M）
  回收单元标称大小，默认 96 MiB。

子系统内命名空间可请求回收单元句柄::

    -device nvme-ns,drive=nvm-1,fdp.ruhs=RUHLIST


RUHLIST 是分号分隔列表，例如 0;1;2;3，可含范围，例如 0;8-15。未指定时，
控制器将其指定的回收单元句柄分配给放置句柄标识符 0。

元数据
--------

命名空间支持独立的 MPTR 元数据和扩展 LBA。

``ms=UINT16`` （默认 0）
  每 LBA 元数据字节数。

``mset=UINT8`` （默认 0）
  1 启用扩展 LBA。

端到端数据保护
--------------------------

根据 mset，支持 DIF 或 DIX 保护信息。

``pi=UINT8`` （默认 0）
  启用类型 1、2 或 3 的保护信息。

``pil=UINT8`` （默认 0）
  控制保护信息在元数据中的位置。1 放在开头，否则放在末尾。

``pif=UINT8`` （默认 0）
  默认 pif=0 使用 16 位 guard。2 启用 64 位 guard，需要至少 16 字节
  元数据。目前不支持 pif=1 的 32 位 guard。

虚拟化增强与 SR-IOV（实验性支持）
-------------------------------------------------------------

nvme 支持单根 I/O 虚拟化与共享及虚拟化增强，SR-IOV 要求关联 nvme-subsys。

以下参数 **可能变化**：

``sriov_max_vfs`` （默认 0）
  最大 PCIe VF 数量，非零使设备报告 SR-IOV 和 ARI 能力。VF 控制器本身
  不报告 SR-IOV。

``sriov_vq_flexible``
  可分配给全部次级控制器的灵活队列资源总数，主控制器私有资源隐式为
  max_ioqpairs - sriov_vq_flexible。

``sriov_vi_flexible``
  可分配的灵活中断资源总数，主控制器私有资源隐式为
  msix_qsize - sriov_vi_flexible。

``sriov_max_vi_per_vf`` （默认 0）
  单个次级控制器最大虚拟中断数，0 表示 sriov_vi_flexible / sriov_max_vfs。

``sriov_max_vq_per_vf`` （默认 0）
  单个次级控制器最大虚拟队列数，0 表示 sriov_vq_flexible / sriov_max_vfs。

最简配置支持一个 VF，分配管理队列、I/O 队列和一个 MSI-X 中断：

.. code-block:: console

   -device nvme-subsys,id=subsys0
   -device nvme,serial=deadbeef,subsys=subsys0,sriov_max_vfs=1,
    sriov_vq_flexible=2,sriov_vi_flexible=1


配置可用次级控制器的最少步骤：

* 从主控制器解绑灵活资源

.. code-block:: console

   nvme virt-mgmt /dev/nvme0 -c 0 -r 1 -a 1 -n 0
   nvme virt-mgmt /dev/nvme0 -c 0 -r 0 -a 1 -n 0


* 对主控制器执行功能级复位，真正释放资源

.. code-block:: console

   echo 1 > /sys/bus/pci/devices/0000:01:00.0/reset


* 启用 VF

.. code-block:: console

   echo 1 > /sys/bus/pci/devices/0000:01:00.0/sriov_numvfs


* 分配灵活资源并设为 ONLINE

.. code-block:: console

   nvme virt-mgmt /dev/nvme0 -c 1 -r 1 -a 8 -n 1
   nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 8 -n 2
   nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 9 -n 0


* 将 NVMe 驱动绑定到 VF

.. code-block:: console

   echo 0000:01:00.1 > /sys/bus/pci/drivers/nvme/bind
