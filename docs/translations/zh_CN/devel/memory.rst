========
内存 API
========

内存 API 建模 QEMU 机器的内存、I/O 总线和控制器，支持描述：

- 普通 RAM；
- 内存映射 I/O（MMIO）；
- 可动态将物理内存区域路由到不同目标的内存控制器。

内存模型支持：

- 跟踪客户机对 RAM 的修改；
- 为 KVM 设置合并访问内存；
- 为 KVM 设置 ioeventfd 区域。

内存表示为 MemoryRegion 对象组成的无环图。
叶节点是 RAM 和 MMIO，其他节点表示总线、内存控制器和重新路由的内存区域。

API 还为每个根节点，以及可能的中间 MemoryRegion 提供 AddressSpace，
表示从 CPU 或设备视角看到的内存。

区域类型
--------

区域均以同一个 C 类型 MemoryRegion 表示，分为多种类型：

- RAM：可暴露给客户机的一段主机内存。
  通常用 memory_region_init_ram() 初始化；
  特殊需求可使用 memory_region_init_resizeable_ram()、
  memory_region_init_ram_from_file() 或 memory_region_init_ram_ptr()。

- MMIO：由主机回调实现的客户机地址范围，每次读写都会调用回调。
  使用 memory_region_init_io()，传入描述回调的 MemoryRegionOps。

- ROM：读取类似 RAM，直接访问主机内存，但禁止写入。
  使用 memory_region_init_rom()。

- ROM 设备：读取类似 RAM，写入类似 MMIO，通过回调处理。
  使用 memory_region_init_rom_device()。

- IOMMU 区域：转换访问地址，并转发到其他目标区域。
  仅用于建模 IOMMU，普通设备不需要。
  使用 memory_region_init_iommu()。

- 容器：包含位于不同偏移的其他区域，便于将多个区域组合为一个单元。
  例如 PCI BAR 可同时包含 RAM 和 MMIO。

  子区域通常不重叠，但有时重叠很有用，
  例如用 MMIO 或 ROM 覆盖部分 RAM 的内存控制器，
  或不禁止设备声明重叠 BAR 的 PCI 控制器。

  纯容器使用 memory_region_init() 初始化。

- 别名：另一区域的子范围，可将同一区域拆分为不连续地址段。
  例如客户机地址空间小于 RAM 容量时的内存分组，或形成 PCI 空洞的内存控制器。
  也可通过别名避免用 `memory_region_add_subregion`
  将原区域添加到多个父节点。

  别名可指向任何区域，包括其他别名，但不得直接或间接指向自身。
  使用 memory_region_init_alias() 初始化。

- 保留区域：主要用于调试，声明不应由 QEMU 自身处理的 I/O 空间。
  典型用途是跟踪启用 KVM 时由主机内核处理的地址段。
  初始化时向 memory_region_init_io() 传入 NULL 回调。

非纯容器的 MMIO、RAM 或 ROM 也可添加子区域。
此时它类似容器，但未被子区域覆盖的地址由自身 MMIO 回调或 RAM 存储处理。
通常可以用纯容器加覆盖全部范围的低优先级背景子区域实现相同效果，
这种方式更清楚，应优先采用。
别名区域不能添加子区域。

迁移
--------

RAM、ROM 和 ROM 设备由主机内存支持，迁移时必须复制到目标。
以下 API 分配主机内存时，也会自动注册迁移：

- memory_region_init_ram()
- memory_region_init_rom()
- memory_region_init_rom_device()

对多数设备和开发板，这正是所需行为。
需要自行管理后备内存迁移的特殊情况，可调用
memory_region_init_ram_flags_nomigrate()，它只初始化区域，迁移由调用者处理。

以下函数仅用于特殊场景：

- memory_region_init_resizeable_ram()
- memory_region_init_ram_from_file()
- memory_region_init_ram_from_fd()
- memory_region_init_ram_ptr()
- memory_region_init_ram_device_ptr()

它们不自动注册后备内存迁移，必要时调用者必须自行管理。

区域名称
--------

构造函数为区域命名。多数名称仅用于调试，
但 RAM 区域名称还标识热迁移段，因此必须保持 ABI 稳定。

区域生命周期
------------

区域通过 ``memory_region_init*()`` 创建，并关联一个作为所有者或父节点的对象。
只要区域对客户机可见，或仍被 vCPU、其他设备使用，QEMU 就保证所有者存活。
例如 address_space_map 与对应 address_space_unmap 之间，所有者不会销毁。

创建后，用 memory_region_add_subregion() 加入地址空间或容器，
用 memory_region_del_subregion() 移除。

只读、脏页日志、合并 MMIO、ioeventfd 等属性可在生命周期内修改，
在区域变为可见时生效，可能立即、稍后或永不生效。

所有者销毁时区域自动销毁。
同一所有者下有多个区域时，API 保证逐个正确分离和终结，但不保证顺序。

若区域位于动态分配的数据结构中，应在 instance_finalize 回调中释放，
例如 hw/vfio/pci.c 的 VFIOMSIXInfo 和 VFIOQuirk。

只要设备或 CPU 仍可能使用区域，就不得销毁它。
一般规则是：不要在设备存活期间动态创建或销毁内存区域，绝不调用 object_unparent()。

否则可能发生：

- memory_region_ref 为所有者取得引用，例如 address_space_map；
- 区域被解除父子关系，不再有所有者；
- address_space_unmap 调用时，原所有者引用泄漏。


例外是别名和容器，可随时调用 object_unparent，
因此也可在设备生命周期内动态创建或销毁。

这是因为它们只帮助构建客户机内存映射，永不直接访问，
memory_region_ref/unref 不会作用于它们，因此不会出现上述问题。
这种例外很少有必要，不建议滥用，但确实有少量使用场景。

创建时传 NULL、“没有所有者”的区域，实际以机器对象为所有者。


重叠区域与优先级
----------------
通常区域不重叠，每个地址只解析到一个目标。
有时允许重叠并控制客户机可见区域很有用，
可通过 memory_region_add_subregion_overlap() 实现。
它允许同一容器内重叠，并指定优先级，由核心选择地址相同时的可见区域，高值优先。
优先级有符号，默认为零：正值可覆盖其他区域，负值可作为底层背景区域。

若高优先级区域是容器或别名，其未映射的空洞会露出低优先级区域。
规则递归适用：内部容器或别名留下的空洞同样如此。

例如容器 A 大小 0x8000，包含 B 和 C。
B 是位于 0x2000、大小 0x4000、优先级 2 的容器；
C 是位于 0、大小 0x6000、优先级 1 的 MMIO。
B 内还有大小均为 0x1000 的 D 和 E，分别位于偏移 0 与 0x2000::

        0      1000   2000   3000   4000   5000   6000   7000   8000
        |------|------|------|------|------|------|------|------|
  A:    [                                                      ]
  C:    [CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC]
  B:                  [                          ]
  D:                  [DDDDD]
  E:                                [EEEEE]


此范围内实际可见的区域为::

  [CCCCCCCCCCCC][DDDDD][CCCCC][EEEEE][CCCCC]


B 比 C 优先，因此 B 的子区域即使与 C 重叠，也出现在平坦映射中。
B 未映射的位置则显示 C。

若 B 自身也有 MMIO 操作，不是纯容器，则 D、E 未处理的 B 内地址由 B 处理，结果为::

  [CCCCCCCCCCCC][DDDDD][BBBBB][EEEEE][BBBBB]


优先级仅在同一容器内比较。
因此负责该容器的设备，通常为总线或内存控制器，
可管理子区域间关系而不影响系统其他部分。
上例 D、E 互不重叠，优先级不重要。
它们覆盖 C 是因为 B 相对 C 更高，D、E 的优先级从不与 C 比较。

可见性
--------
客户机访问地址时，内存核心按以下规则选择区域：

- 按优先级降序检查根区域的全部直接子区域。

  - 地址不在该子区域的偏移和大小范围内，则跳过。
  - 子区域为 RAM 或 MMIO 叶节点，结束搜索并返回它。
  - 子区域为容器，扣除其偏移后，在内部递归执行相同算法。
  - 子区域为别名，按子区域偏移和别名偏移调整地址后，在目标区域继续搜索。
  - 容器或别名内部因空洞而未匹配时，若该容器自身具有 MMIO 或 RAM，
    返回容器本身；否则按优先级继续检查下一个子区域。

- 所有子区域均不匹配时，搜索结束，返回无匹配。

内存映射示例
------------

::

  system_memory: container@0-2^48-1
   |
   +---- lomem: alias@0-0xdfffffff ---> #ram (0-0xdfffffff)
   |
   +---- himem: alias@0x100000000-0x11fffffff ---> #ram (0xe0000000-0xffffffff)
   |
   +---- vga-window: alias@0xa0000-0xbffff ---> #pci (0xa0000-0xbffff)
   |      (prio 1)
   |
   +---- pci-hole: alias@0xe0000000-0xffffffff ---> #pci (0xe0000000-0xffffffff)

  pci (0-2^32-1)
   |
   +--- vga-area: container@0xa0000-0xbffff
   |      |
   |      +--- alias@0x00000-0x7fff  ---> #vram (0x010000-0x017fff)
   |      |
   |      +--- alias@0x08000-0xffff  ---> #vram (0x020000-0x027fff)
   |
   +---- vram: ram@0xe1000000-0xe1ffffff
   |
   +---- vga-mmio: mmio@0xe2000000-0xe200ffff

  ram: ram@0x00000000-0xffffffff


这是简化的 PC 内存映射。
4 GB RAM 通过两个别名映射到系统地址空间：lomem 将前 3.5 GB 一一映射，
himem 将最后 0.5 GB 映射到 4 GB 地址处。
这样预留 0.5 GB 的 PCI 空洞，使 32 位 PCI 总线可用于具有 4 GB 内存的系统。

内存控制器把 640K～768K 地址转向 PCI 空间。
通过较高优先级的 vga-window 别名建模，覆盖相同地址的 RAM。
客户机可配置控制器移除 VGA 窗口，对应删除别名、露出下层 RAM。

PCI 地址空间不是系统空间的直接子节点，因为只希望暴露部分范围，使用别名实现。
它有两个子区域：vga-area 建模传统 VGA 窗口，由两个 32K 内存分组占据，
分别指向帧缓冲区的两部分。
此外，VRAM 作为 BAR 映射到 e1000000，其后还有包含 MMIO 寄存器的 BAR。

客户机若把 BAR 映射到 PCI 空洞之外，该 BAR 不可见，
因为 pci-hole 别名将可见范围限制为 0.5 GB。

MMIO 操作
---------

MMIO 区域提供 ->read() 和 ->write() 回调，满足多数设备需求。
若设备行为取决于内存事务属性，或需要返回总线错误而非成功完成，
可以使用 ->read_with_attrs() 和 ->write_with_attrs()。

还可通过以下约束控制调用方式：

- .valid.min_access_size、.valid.max_access_size：设备接受的访问字节数范围。
  超出时按设备和总线规则处理，例如忽略或触发机器检查。
- .valid.unaligned：被建模设备是否支持非对齐访问。
  为假时，按总线或 CPU 专用规则处理。
- .impl.min_access_size、.impl.max_access_size：实现实际支持的访问大小。
  其他大小由已有访问模拟，例如最大为 1 时，以四次单字节写模拟四字节写。
- .impl.unaligned：实现是否支持非对齐访问。
  为假时，用两次对齐访问模拟。

API 参考
--------

.. kernel-doc:: include/system/memory.h
