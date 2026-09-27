..
   Copyright (c) 2017 Linaro Limited
   Written by Peter Maydell

==============
加载和存储 API
==============

QEMU 内部有多组加载和存储函数。
本文列出这些函数并说明适用场景，不详细描述各 API；
具体接口说明应查阅相应头文件的文档注释。


``ld*_p and st*_p``
~~~~~~~~~~~~~~~~~~~

这些函数操作主机指针，适用于已取得主机内存指针的情况，
该内存可能对应客户机 RAM 或本地缓冲区。
它们按所需字节序访问，并正确处理可能未对齐的指针。

函数命名模式如下：

加载： ``ld{sign}{size}_{endian}_p(ptr)``

存储： ``st{size}_{endian}_p(ptr, val)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``24``：24 位。
 - ``l``：32 位。
 - ``q``：64 位。

``endian``
 - ``he``：主机字节序。
 - ``be``：大端。
 - ``le``：小端。

按目标字节序访问时，省略 ``_{endian}`` 中缀。

目标字节序访问函数只供按目标分别编译的源码使用。

也有把大小作为参数的函数：

加载： ``ldn_{endian}_p(ptr, sz)``

它从 ptr 按指定字节序无符号读取 sz 字节，以 uint64_t 返回。

存储： ``stn_{endian}_p(ptr, sz, val)``

它将 val 按指定字节序以 sz 字节写入 ptr。


用于 git grep 的正则表达式：
 - ``\<ld[us]\?[bwlq]\(_[hbl]e\)\?_p\>``
 - ``\<st[bwlq]\(_[hbl]e\)\?_p\>``
 - ``\<st24\(_[hbl]e\)\?_p\>``
 - ``\<ldn\(_[hbl]e\)\?_p\>``
 - ``\<stn\(_[hbl]e\)\?_p\>``

``cpu_{ld,st}*_mmu``
~~~~~~~~~~~~~~~~~~~~

这些函数操作客户机虚拟地址，另接收控制地址转换的 MMU 索引上下文，
以及包含对齐要求等信息的 MemOp。
MemOp 与 MMU 索引合并为一个 MemOpIdx 参数。

索引含义取决于目标架构。
例如辅助函数要求始终以非特权方式访问，而非按当前客户机 CPU 状态默认访问时，
就可能需要指定特定索引。

这些函数可能触发客户机 CPU 异常，例如对齐或 MMU 错误，
更新客户机状态并通过 longjmp 跳出调用。
因此只能用于实现客户机 CPU 模拟的代码。

retaddr 传给 cpu_restore_state()，控制异常时客户机 CPU 状态的回溯恢复。
值应为 0，表示状态已同步，或为顶层 HELPER(foo) 中 GETPC() 的结果，
即指向生成代码的返回地址 [#gpc]_。

.. [#gpc] GETPC() 必须谨慎使用，在顶层 HELPER(foo) 之外调用会导致非预期行为。
          应在顶层辅助函数读取，再按需传给其调用的其他函数。

函数命名模式如下：

加载： ``cpu_ld{size}{end}_mmu(env, ptr, oi, retaddr)``

存储： ``cpu_st{size}{end}_mmu(env, ptr, val, oi, retaddr)``

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``end``
 - 空：用于目标字节序或 8 位访问。
 - ``_be``：大端。
 - ``_le``：小端。

用于 git grep 的正则表达式：
 - ``\<cpu_ld[bwlq]\(_[bl]e\)\?_mmu\>``
 - ``\<cpu_st[bwlq]\(_[bl]e\)\?_mmu\>``


``cpu_{ld,st}*_mmuidx_ra``
~~~~~~~~~~~~~~~~~~~~~~~~~~

类似 cpu_{ld,st}_mmu，但 mmuidx 不与 MemOp 合并，
因此不提供也不强制对齐要求。

函数命名模式如下：

加载： ``cpu_ld{sign}{size}{end}_mmuidx_ra(env, ptr, mmuidx, retaddr)``

存储： ``cpu_st{size}{end}_mmuidx_ra(env, ptr, val, mmuidx, retaddr)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``end``
 - 空：用于目标字节序或 8 位访问。
 - ``_be``：大端。
 - ``_le``：小端。

用于 git grep 的正则表达式：
 - ``\<cpu_ld[us]\?[bwlq]\(_[bl]e\)\?_mmuidx_ra\>``
 - ``\<cpu_st[bwlq]\(_[bl]e\)\?_mmuidx_ra\>``

``cpu_{ld,st}*_data_ra``
~~~~~~~~~~~~~~~~~~~~~~~~

类似 cpu_{ld,st}_mmuidx_ra，但 mmuidx 来自客户机 CPU 当前模式，
由 cpu_mmu_index(env, false) 决定。

辅助函数按客户机虚拟地址访问时，通常首选这组函数，
除非需要非默认上下文或强制对齐。

函数命名模式如下：

加载： ``cpu_ld{sign}{size}{end}_data_ra(env, ptr, ra)``

存储： ``cpu_st{size}{end}_data_ra(env, ptr, val, ra)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``end``
 - 空：用于目标字节序或 8 位访问。
 - ``_be``：大端。
 - ``_le``：小端。

用于 git grep 的正则表达式：
 - ``\<cpu_ld[us]\?[bwlq]\(_[bl]e\)\?_data_ra\>``
 - ``\<cpu_st[bwlq]\(_[bl]e\)\?_data_ra\>``

``cpu_{ld,st}*_data``
~~~~~~~~~~~~~~~~~~~~~

类似 cpu_{ld,st}_data_ra，但 retaddr 为 0，因此不回溯恢复客户机 CPU 状态。

因此只适用于翻译器已保存全部必要 CPU 状态的辅助函数。
从 do_interrupt 等 CPU 钩子调用，或能确定翻译器必然已保存所有状态时，应使用它们。

函数命名模式如下：

加载： ``cpu_ld{sign}{size}{end}_data(env, ptr)``

存储： ``cpu_st{size}{end}_data(env, ptr, val)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``end``
 - 空：用于目标字节序或 8 位访问。
 - ``_be``：大端。
 - ``_le``：小端。

用于 git grep 的正则表达式：
 - ``\<cpu_ld[us]\?[bwlq]\(_[bl]e\)\?_data\>``
 - ``\<cpu_st[bwlq]\(_[bl]e\)\?_data\+\>``

``cpu_ld*_code_mmu``
~~~~~~~~~~~~~~~~~~~~

类似 cpu_{ld,st}*_mmu，但执行用于指令执行的读取。
产生的客户机异常会标识为指令执行故障，而非数据读取故障。

翻译期间一般不应直接使用，应使用同时处理跟踪插件的封装函数。

函数命名模式如下：

加载： ``cpu_ld{sign}{size}_code_mmu(env, addr, oi, retaddr)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

用于 git grep 的正则表达式：
 - ``\<cpu_ld[us]\?[bwlq]_code_mmu\>``

``translator_ld*``
~~~~~~~~~~~~~~~~~~

封装 cpu_ld*_code_mmu，并执行跟踪插件所需操作。
只能在翻译器的 translate_insn 回调期间调用。

以 _swap 结尾的函数在参数为 true 时，
按 TARGET_BIG_ENDIAN 所确定的客户机原生字节序的相反顺序返回值。

函数命名模式如下：

加载： ``translator_ld{sign}{size}(env, ptr)``

字节交换： ``translator_ld{sign}{size}_swap(env, ptr, swap)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

用于 git grep 的正则表达式：
 - ``\<translator_ld[us]\?[bwlq]\(_swap\)\?\>``

``helper_{ld,st}*_mmu``
~~~~~~~~~~~~~~~~~~~~~~~

主要由 TCG 后端生成的代码调用。
类似 cpu_{ld,st}_mmu，使用指定 MemOpIdx 按客户机虚拟地址访问。

区别是字节序仅来自 MemOpIdx，
加载还会把返回值扩展到主机通用寄存器大小，即 tcg_target_ulong。

加载： ``helper_ld{sign}{size}_mmu(env, addr, opindex, retaddr)``

存储： ``helper_{size}_mmu(env, addr, val, opindex, retaddr)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。
 - ``s``：有符号。

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

用于 git grep 的正则表达式：
 - ``\<helper_ld[us]\?[bwlq]_mmu\>``
 - ``\<helper_st[bwlq]_mmu\>``

``address_space_*``
~~~~~~~~~~~~~~~~~~~

这是模拟 CPU 或设备内存访问的主要函数。
接收 AddressSpace，表示设备或 CPU 看到的内存视图，
通常对应硬件总线或互连结构的主设备端。

每个 CPU 都有 AddressSpace，部分 CPU 有多个，
例如支持 TrustZone 的 Arm 分别为安全世界和非安全世界设置地址空间。
能执行 DMA 的设备通常也应有自己的 AddressSpace。
系统地址空间一般包含全部 CPU 可见的设备和内存。
部分旧设备模型直接使用它，而未正确建模自身地址空间。

提供按字节缓冲区读写及单个数据项加载／存储两类接口。

调用者均需提供 MemTxAttrs 指定总线事务属性，
并可通过 MemTxResult 返回码检查是否成功。

``address_space_read(address_space, addr, attrs, buf, len)``

``address_space_write(address_space, addr, attrs, buf, len)``

``address_space_rw(address_space, addr, attrs, buf, len, is_write)``

``address_space_ld{sign}{size}_{endian}(address_space, addr, attrs, txresult)``

``address_space_st{size}_{endian}(address_space, addr, val, attrs, txresult)``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。

（不提供有符号加载。）

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``endian``
 - ``le``：小端。
 - ``be``：大端。

字节访问省略 ``_{endian}`` 后缀。

用于 git grep 的正则表达式：
 - ``\<address_space_\(read\|write\|rw\)\>``
 - ``\<address_space_ldu\?[bwql]\(_[lb]e\)\?\>``
 - ``\<address_space_st[bwql]\(_[lb]e\)\?\>``

``address_space_write_rom``
~~~~~~~~~~~~~~~~~~~~~~~~~~~

类似 address_space_write，按物理地址写入，
但对 ROM 也会修改内容，而客户机 CPU 的普通 ROM 写入会被忽略。
用于 GDB 调试桩写入或 ROM 初始加载等非客户机写操作。

写入范围中针对设备的部分会静默忽略，只写实际 RAM 和 ROM。

用于 git grep 的正则表达式：
 - ``address_space_write_rom``

``{ld,st}*_phys``
~~~~~~~~~~~~~~~~~

类似 address_space_{ld,st}*，但事务属性始终为 MEMTXATTRS_UNSPECIFIED，
并忽略事务成败。

由于忽略结果，新代码不应使用，
除非能确定访问方 CPU 或设备本来就没有报告此类错误的机制。

``load: ld{sign}{size}_{endian}_phys``

``store: st{size}_{endian}_phys``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。

（不提供有符号加载。）

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``endian``
 - ``le``：小端。
 - ``be``：大端。

字节访问省略 ``_{endian}_`` 中缀。

用于 git grep 的正则表达式：
 - ``\<ldu\?[bwlq]\(_[bl]e\)\?_phys\>``
 - ``\<st[bwlq]\(_[bl]e\)\?_phys\>``

``physical_memory_*``
~~~~~~~~~~~~~~~~~~~~~

这些便捷函数类似 address_space_*，但固定操作旧式全局
&address_space_memory，并非所有机器都使用该空间。
属性始终为 MEMTXATTRS_UNSPECIFIED，且忽略事务成败。
预期供硬件设备模型使用，但新代码最好避免，原因是：

* 读写失败时，很可能需要正确模拟相应行为；
* 设备通常应在自己的 AddressSpace 操作，而非系统地址空间；
* 部分机器根本不使用这个全局地址空间。

``physical_memory_read``

``physical_memory_write``

用于 git grep 的正则表达式：
 - ``\<physical_memory_\(read\|write\)\>``

``cpu_memory_rw_debug``
~~~~~~~~~~~~~~~~~~~~~~~

为调试目的按虚拟地址访问 CPU 内存。

供 GDB 桩及类似代码使用。根据指定 CPU 当前设置通过 MMU 查找，
将虚拟地址转为物理地址，再执行访问。
读取用 address_space_rw，写入用 address_space_write_rom。
因此写 ROM 时会实际修改，而普通客户机 CPU 写入会被忽略。

``cpu_memory_rw_debug``

``dma_memory_*``
~~~~~~~~~~~~~~~~

类似 address_space_*，但先执行 DMA 屏障。

**待办：** 应说明何时需要 DMA 屏障，何时可直接用 address_space_*，
并确认现有代码处理正确。

``dma_memory_read``

``dma_memory_write``

``dma_memory_rw``

用于 git grep 的正则表达式：
 - ``\<dma_memory_\(read\|write\|rw\)\>``
 - ``\<ldu\?[bwlq]\(_[bl]e\)\?_dma\>``
 - ``\<st[bwlq]\(_[bl]e\)\?_dma\>``

``pci_dma_*`` 与 ``{ld,st}*_pci_dma``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

专供作为总线主设备发起访问的 PCI 设备模型使用。
传入 ``PCIDevice *`` 后，它们在该设备正确的地址空间执行 dma_memory_* 操作。

``pci_dma_read``

``pci_dma_write``

``pci_dma_rw``

``load: ld{sign}{size}_{endian}_pci_dma``

``store: st{size}_{endian}_pci_dma``

``sign``
 - 空：用于 32 或 64 位。
 - ``u``：无符号。

（不提供有符号加载。）

``size``
 - ``b``：8 位。
 - ``w``：16 位。
 - ``l``：32 位。
 - ``q``：64 位。

``endian``
 - ``le``：小端。
 - ``be``：大端。

字节访问省略 ``_{endian}_`` 中缀。

用于 git grep 的正则表达式：
 - ``\<pci_dma_\(read\|write\|rw\)\>``
 - ``\<ldu\?[bwlq]\(_[bl]e\)\?_pci_dma\>``
 - ``\<st[bwlq]\(_[bl]e\)\?_pci_dma\>``
