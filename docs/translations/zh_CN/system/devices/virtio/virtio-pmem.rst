VirtIO 持久内存
========================

本文介绍 virtio pmem，这是在普通、非 NVDIMM 存储上的半虚拟化持久内存设备。

使用场景
--------

它绕过客户机页缓存，直接使用主机页缓存，减少客户机内存占用，并使主机在
内存压力下有效决定回收。

与 NVDIMM 模拟的比较
-----------------------------------------------------

普通主机存储上的 NVDIMM 模拟不保证客户机写入持久化，因为设备规范没有
定义相应语义。virtio pmem 则为非 NVDIMM 存储提供写入持久性。

用法
-----------------

使用 memory-backend-file 创建，例如::

    -object memory-backend-file,id=mem1,share,mem-path=./virtio_pmem.img,size=4G
    -device virtio-pmem-pci,memdev=mem1,id=nv1


其中：

  - memory-backend-file 创建指定大小的后端文件。

  - virtio-pmem-pci 创建由该后端提供存储的 PCI 设备。

提供多组 -object 和 -device 即可创建多个设备。

热插入
-------

可通过监视器热插入，先用 object_add 添加内存后端，再用 device_add 添加设备。

例如再添加 4GB::

 (qemu) object_add memory-backend-file,id=mem2,share=on,mem-path=virtio_pmem2.img,size=4G
 (qemu) device_add virtio-pmem-pci,id=virtio_pmem2,memdev=mem2


客户机数据持久性
----------------------

非 NVDIMM 存储需要客户机用户程序执行 fsync/msync。这与真实 NVDIMM 不同，
后者无需额外同步。同步将写入落到主机文件，否则仍停留页缓存，断电可能丢失。

不支持 MAP_SYNC mmap 标志，以提示应用执行 fsync 保证持久性。

限制
-----------

- 不支持真实 NVDIMM 后端。
- 不支持热移除。
- 不支持 ACPI NVDIMM 的 region、namespace 等功能。
- 不支持 ndctl。
