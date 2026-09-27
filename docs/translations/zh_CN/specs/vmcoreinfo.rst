========================================
VMCoreInfo 设备
========================================

``-device vmcoreinfo`` 会创建一个 ``fw_cfg`` 条目，供客户机存储转储详情。

``etc/vmcoreinfo``
========================================

客户机可用此 ``fw_cfg`` 条目为 QEMU 转储添加详细信息。

条目长 16 字节，采用小端布局::

    #define VMCOREINFO_FORMAT_NONE 0x0
    #define VMCOREINFO_FORMAT_ELF 0x1

    struct FWCfgVMCoreInfo {
        uint16_t host_format;  /* formats host supports */
        uint16_t guest_format; /* format guest supplies */
        uint32_t size;         /* size of vmcoreinfo region */
        uint64_t paddr;        /* physical address of vmcoreinfo region */
    };

仅完整写入 16 字节才被视为有效，并继续处理条目值。

向 ``guest_format`` 写入 0，会禁用 vmcoreinfo 条目值和内容的后续处理。

可以写入主机不支持的 ``guest_format``，此时 QEMU 可忽略条目数据，
但仍可通过调试器访问 ``vmcoreinfo_realize::vmcoreinfo_state``。

格式与内容
========================================

截至 QEMU 2.11，仅支持 ``VMCOREINFO_FORMAT_ELF``。

条目指定一个 ELF note 的位置和大小，它会追加到 QEMU 转储。

note 的格式／类别必须与目标位数一致，大小必须小于 1Mb。

若 ELF note 名称为 ``VMCOREINFO``，则应为 Linux vmcoreinfo note，
格式见 `内核文档
<https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-kernel-vmcoreinfo>`_。
此时 QEMU 转储代码将内容作为 key=value 文本读取，查找 ``NUMBER(phys_base)``。
预期此值比针对架构推测的值更准确。对于启用 KASLR 的客户机，
若旧工具不能处理 ``VMCOREINFO`` note，此功能尤其有用。
