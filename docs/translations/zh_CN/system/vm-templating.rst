QEMU 虚拟机模板
==================

本文介绍虚拟机模板的使用。

目前重点是内存，不涉及其他状态的保存和恢复，例如配合 x-ignore-shared
迁移到文件。

概述
--------

模板虚拟机作为创建新虚拟机的起点，可以快速高效复制，缩短启动时间并降低
内存占用。

概念上先冻结状态，再以其为基础创建新机器。操作系统的写时复制机制允许新
机器读取模板内存，但修改保持私有，不影响原模板或其他副本。

安全警告
----------------------

模板实质上克隆虚拟机，因此本应唯一的 UUID、网卡 MAC，以及客户机中的
机器 ID、SSH 密钥、证书等不再唯一，可能产生安全问题。

请了解影响并按具体用途缓解，例如使用 vmgenid、热插拔网卡等。

内存配置
--------------------

创建模板时，必须让 RAM 保存在文件中，供后续复用。

用 memory-backend-file 提供内存，share=on 将修改写回文件，readonly=off
以可写方式打开；后者是默认值。

以下创建 2GB 虚拟机，将 RAM 存于 template：

.. parsed-literal::

    |qemu_system| [...] -m 2g \\
        -object memory-backend-file,id=pc.ram,mem-path=template,size=2g,share=on,... \\
        -machine q35,memory-backend=pc.ram


若使用 vNUMA、DIMM 等多个后端，全部都要相应配置。

达到预期状态后暂停，保存其他状态，使当前 RAM 保留在文件中。

创建基于模板的新机器时，内存仍以该文件为基础，但不得修改文件内容。

使用 memory-backend-file，share=off 保持修改私有，readonly=on 只读打开，
rom=off 则不将客户机内存设为只读。share=off 为默认；其他虚拟机状态需另行恢复。

以下基于已有 2GB template 文件创建机器：

.. parsed-literal::

    |qemu_system| [...] -m 2g \\
        -object memory-backend-file,id=pc.ram,mem-path=template,size=2g,readonly=on,rom=off,... \\
        -machine q35,memory-backend=pc.ram


多个内存后端时，全部应相应配置。

无论创建模板还是从模板启动新机器，都不能使用 -mem-path。

不兼容功能
---------------------

部分功能需要修改底层文件以丢弃 RAM，或与其他进程真正共享内存，因此不兼容。

vhost-user 和多进程 QEMU
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

它们依赖共享内存，而模板虽然基于文件，却使用 share=off，并不真正共享。

virtio-balloon
~~~~~~~~~~~~~~

气球膨胀和空闲页报告无法丢弃 RAM，会反复报错。仍可用 virtio-balloon
报告统计等，但应禁用空闲页报告，且不要让气球膨胀。

virtio-mem
~~~~~~~~~~

virtio-mem 无法丢弃其管理的 RAM，会在设备实现初期失败。要与模板配合，
可在新虚拟机热插入该设备，或让模板的 requested-size=0，不通过它提供
内存，且不要将模板文件用作其后端。

虚拟机迁移
~~~~~~~~~~~~

x-release-ram 依赖在源端丢弃已迁移 RAM，同样会反复报错。

后复制热迁移在目的端丢弃内存时就会失败，拒绝启用。即使普通情况下，后复制
也通常只支持 shmem/tmpfs、hugetlbfs 等特定文件系统。
