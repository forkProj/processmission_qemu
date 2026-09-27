.. _Arm Emulation:

A-profile CPU 架构支持
==================================

QEMU 的 TCG 模拟支持 A-profile 架构的 Armv5、Armv6、Armv7、Armv8 和 Armv9 版本，还支持以下架构扩展：

- FEAT_AA32（PE 支持 AArch32）
- FEAT_AA32BF16（AArch32 BFloat16 指令）
- FEAT_AA32EL0（在 EL0 支持 AArch32）
- FEAT_AA32EL1（在 EL1 支持 AArch32）
- FEAT_AA32EL2（在 EL2 支持 AArch32）
- FEAT_AA32EL3（在 EL3 支持 AArch32）
- FEAT_AA32HPD（AArch32 层次权限禁用）
- FEAT_AA32I8MM（AArch32 Int8 矩阵乘法指令）
- FEAT_AA64（PE 在最近一次重启后使用 AArch64）
- FEAT_AA64EL0（在 EL0 支持 AArch64）
- FEAT_AA64EL1（在 EL1 支持 AArch64）
- FEAT_AA64EL2（在 EL2 支持 AArch64）
- FEAT_AA64EL3（在 EL3 支持 AArch64）
- FEAT_AdvSIMD（高级 SIMD 扩展）
- FEAT_AES（AESD 和 AESE 指令）
- FEAT_AFP（替代浮点行为）
- FEAT_AIE（内存属性索引增强）
- FEAT_Armv9_Crypto（Armv9 密码扩展）
- FEAT_ASID16（16 位 ASID）
- FEAT_ASID2（并发使用两个 ASID）
- FEAT_ATS1A（忽略第一级权限的地址转换操作）
- FEAT_BBML1（转换表先断后建级别 1）
- FEAT_BBML2（转换表先断后建级别 2）
- FEAT_BF16（AArch64 BFloat16 指令）
- FEAT_BigEnd（在 EL1 及更高级别支持大端）
- FEAT_BigEndEL0（在 EL0 支持大端）
- FEAT_BTI（分支目标标识）
- FEAT_CCIDX（扩展缓存索引）
- FEAT_CHK（检查功能状态）
- FEAT_CMOW（缓存维护权限控制）
- FEAT_CMPBR（比较与分支指令）
- FEAT_CRC32（CRC32 指令）
- FEAT_Crypto（密码扩展）
- FEAT_CSSC（常用短序列压缩指令）
- FEAT_CSV2（缓存推测变体 2）
- FEAT_CSV2_1p1（缓存推测变体 2，版本 1.1）
- FEAT_CSV2_1p2（缓存推测变体 2，版本 1.2）
- FEAT_CSV2_2（缓存推测变体 2，版本 2）
- FEAT_CSV2_3（缓存推测变体 2，版本 3）
- FEAT_CSV3（缓存推测变体 3）
- FEAT_DGH（数据汇集提示）
- FEAT_DIT（数据无关时序指令）
- FEAT_DoubleLock（双重锁定）
- FEAT_DPB（DC CVAP 指令）
- FEAT_DPB2（DC CVADP 指令）
- FEAT_Debugv8p1（带 VHE 的调试）
- FEAT_Debugv8p2（v8.2 调试变更）
- FEAT_Debugv8p4（v8.4 调试变更）
- FEAT_Debugv8p8（v8.8 调试变更）
- FEAT_DotProd（高级 SIMD 点积指令）
- FEAT_DoubleFault（双重故障扩展）
- FEAT_E0PD（阻止 EL0 访问地址映射的半区）
- FEAT_E2H0（HCR_EL2.E2H 编程）
- FEAT_EBF16（AArch64 扩展 BFloat16 指令）
- FEAT_ECV（增强计数器虚拟化）
- FEAT_ECV_POFF（增强计数器虚拟化物理偏移）
- FEAT_EL0（支持在 EL0 执行）
- FEAT_EL1（支持在 EL1 执行）
- FEAT_EL2（支持在 EL2 执行）
- FEAT_EL3（支持在 EL3 执行）
- FEAT_EPAC（增强指针认证）
- FEAT_ETS2（增强地址转换同步）
- FEAT_EVT（增强虚拟化陷入）
- FEAT_EVT2（增强虚拟化陷入 2）
- FEAT_F8F16MM（8 位浮点矩阵乘累加，结果为半精度）
- FEAT_F8F32MM（8 位浮点矩阵乘累加，结果为单精度）
- FEAT_F32MM（单精度矩阵乘法）
- FEAT_F64MM（双精度矩阵乘法）
- FEAT_FAMINMAX（浮点最大和最小绝对值指令）
- FEAT_FCMA（浮点复数指令）
- FEAT_FGT（细粒度陷入）
- FEAT_FHM（浮点半精度乘法指令）
- FEAT_FP（浮点扩展）
- FEAT_FP16（半精度浮点数据处理）
- FEAT_FP8（FP8 转换指令）
- FEAT_FP8DOT2（FP8 二路点积转半精度指令）
- FEAT_FP8DOT4（FP8 四路点积转单精度指令）
- FEAT_FP8FMA（FP8 乘累加转半精度和单精度指令）
- FEAT_FPAC（AUT* 指令触发故障）
- FEAT_FPACCOMBINE（组合指针认证指令触发故障）
- FEAT_FPACC_SPEC（组合指针认证指令的推测行为）
- FEAT_FPMR（浮点模式寄存器）
- FEAT_FPRCVT（在标量 FP 寄存器中进行浮点与整数双向转换）
- FEAT_FRINTTS（浮点转整数指令）
- FEAT_FlagM（标志操作指令 v2）
- FEAT_FlagM2（标志操作指令增强）
- FEAT_GCS（受保护控制栈扩展）
- FEAT_GTG（客户机地址转换粒度大小）
- FEAT_HAF（硬件管理访问标志）
- FEAT_HAFDBS（硬件管理访问标志和脏位状态）
- FEAT_HBC（带提示的条件分支）
- FEAT_HCX（支持 HCRX_EL2 寄存器）
- FEAT_HPDS（层次权限禁用）
- FEAT_HPDS2（转换表基于页的硬件属性）
- FEAT_HPMN0（将 MDCR_EL2.HPMN 设为零）
- FEAT_I8MM（AArch64 Int8 矩阵乘法指令）
- FEAT_IDST（ID 空间陷入处理）
- FEAT_IESB（隐式错误同步事件）
- FEAT_IVIPT（IVIPT 扩展）
- FEAT_JSCVT（JavaScript 转换指令）
- FEAT_LittleEnd（在 EL1 及更高级别支持小端）
- FEAT_LittleEndEL0（在 EL0 支持小端）
- FEAT_LOR（有限排序区域）
- FEAT_LPA（大物理地址空间）
- FEAT_LPA2（大物理与虚拟地址空间 v2）
- FEAT_LRCPC（加载获取 RCpc 指令）
- FEAT_LRCPC2（加载获取 RCpc 指令 v2）
- FEAT_LSE（大型系统扩展）
- FEAT_LSE2（大型系统扩展 v2）
- FEAT_LSE128（128 位原子操作）
- FEAT_LUT（使用 2 位和 4 位索引的查找表指令）
- FEAT_LVA（大虚拟地址空间）
- FEAT_MEC（内存加密上下文）

  * 此实现仅提供寄存器，不执行加密。
- FEAT_MixedEnd（混合端序支持）
- FEAT_MixedEndEL0（EL0 混合端序支持）
- FEAT_MOPS（内存操作标准化）
- FEAT_MTE（内存标记扩展）
- FEAT_MTE2（内存标记扩展）
- FEAT_MTE3（MTE 非对称故障处理）
- FEAT_MTE4（增强内存标记扩展）
- FEAT_MTE_ASYM_FAULT（内存标记非对称故障）
- FEAT_MTE_ASYNC（异步报告标记检查故障）
- FEAT_MTE_CANONICAL_TAGS（规范标记检查）
- FEAT_MTE_NO_ADDRESS_TAGS（禁用地址标记）
- FEAT_MTE_PERM（NoTagAccess 内存属性）
- FEAT_MTE_STORE_ONLY（仅存储操作的标记检查）
- FEAT_MTE_TAGGED_FAR（标记检查故障的完整地址报告）
- FEAT_NMI（不可屏蔽中断）
- FEAT_NV（嵌套虚拟化）
- FEAT_NV2（增强嵌套虚拟化支持）
- FEAT_PACIMP（指针认证——由实现定义的算法）
- FEAT_PACQARMA3（指针认证——QARMA3 算法）
- FEAT_PACQARMA5（指针认证——QARMA5 算法）
- FEAT_PAN（禁止特权访问）
- FEAT_PAN2（受 PSTATE.PAN 影响的 AT S1E1R 和 AT S1E1W 指令变体）
- FEAT_PAN3（支持 SCTLR_ELx.EPAN）
- FEAT_PAuth（指针认证）
- FEAT_PAuth2（指针认证增强）
- FEAT_PMULL（PMULL、PMULL2 指令）
- FEAT_PMUv3（PMU 扩展第 3 版）
- FEAT_PMUv3p1（PMU 扩展 v3.1）
- FEAT_PMUv3p4（PMU 扩展 v3.4）
- FEAT_PMUv3p5（PMU 扩展 v3.5）
- FEAT_RAS（可靠性、可用性和可维护性）
- FEAT_RASv1p1（RAS 扩展 v1.1）
- FEAT_RDM（高级 SIMD 舍入倍乘累加指令）
- FEAT_RME（Realm 管理扩展）（注意：QEMU 中的支持为实验性）
- FEAT_RME_GPC2（RME 粒度保护检查 2 扩展）
- FEAT_RNG（随机数生成器）
- FEAT_RNG_TRAP（RNDR/RNDRRS 陷入支持）
- FEAT_RPRES（提高 FRECPE 和 FRSQRTE 的精度）
- FEAT_S1PIE（第一级权限间接寻址）
- FEAT_S2FWB（第二级强制写回）
- FEAT_S2PIE（第二级权限间接寻址）
- FEAT_S2TGran16K（第二级支持 16KB 内存转换粒度）
- FEAT_S2TGran4K（第二级支持 4KB 内存转换粒度）
- FEAT_S2TGran64K（第二级支持 64KB 内存转换粒度）
- FEAT_SB（推测屏障）
- FEAT_SCTLR2（SCTLR_ELx 扩展）
- FEAT_Secure（支持安全状态）
- FEAT_SEL2（安全 EL2）
- FEAT_SHA1（SHA1 指令）
- FEAT_SHA256（SHA256 指令）
- FEAT_SHA3（高级 SIMD SHA3 指令）
- FEAT_SHA512（高级 SIMD SHA512 指令）
- FEAT_SM3（高级 SIMD SM3 指令）
- FEAT_SM4（高级 SIMD SM4 指令）
- FEAT_SME（可伸缩矩阵扩展）
- FEAT_SME2（可伸缩矩阵扩展第 2 版）
- FEAT_SME2p1（可伸缩矩阵扩展第 2.1 版）
- FEAT_SME_B16B16（SME2 的非拓宽 BFloat16 算术）
- FEAT_SME_FA64（流式 SVE 模式中的完整 A64 指令集）
- FEAT_SME_F16F16（SME2 的非拓宽半精度 FP16 算术）
- FEAT_SME_F64F64（双精度浮点外积指令）
- FEAT_SME_F8F16（以 SME2 ZA 为目标的 FP8 乘累加、点积和外积转半精度指令）
- FEAT_SME_F8F32（以 SME2 ZA 为目标的 FP8 乘累加、点积和外积转单精度指令）
- FEAT_SME_I16I64（16 位到 64 位整数拓宽外积指令）
- FEAT_SME_LUTv2（使用 4 位索引和 8 位元素的查找表指令）
- FEAT_SME_MOP4（四分之一块外积指令）
- FEAT_SSVE_AES（流式 SVE 模式的高级加密标准和 128 位多项式长乘法指令）
- FEAT_SSVE_FEXPA（流式 FEXPA 指令）
- FEAT_SSVE_FP8DOT2（流式 SVE 模式中的 SVE2 FP8 二路点积转半精度指令）
- FEAT_SSVE_FP8DOT4（流式 SVE 模式中的 SVE2 FP8 四路点积转单精度指令）
- FEAT_SSVE_FP8FMA（流式 SVE 模式中的 SVE2 FP8 乘累加转半精度和单精度指令）
- FEAT_SVE（可伸缩向量扩展）
- FEAT_SVE_AES（可伸缩向量 AES 指令）
- FEAT_SVE_B16B16（SVE2 的非拓宽 BFloat16 算术）
- FEAT_SVE_BitPerm（可伸缩向量位排列指令）
- FEAT_SVE_PMULL128（可伸缩向量 PMULL 指令）
- FEAT_SVE_SHA3（可伸缩向量 SHA3 指令）
- FEAT_SVE_SM4（可伸缩向量 SM4 指令）
- FEAT_SVE2（可伸缩向量扩展第 2 版）
- FEAT_SVE2p1（可伸缩向量扩展第 2.1 版）
- FEAT_SPECRES（推测限制指令）
- FEAT_SSBS（推测存储绕过安全）
- FEAT_SSBS2（SSBS 第 2 版的 MRS 和 MSR 指令）
- FEAT_TCR2（支持 TCR2_ELx）
- FEAT_TGran16K（第一级支持 16KB 内存转换粒度）
- FEAT_TGran4K（第一级支持 4KB 内存转换粒度）
- FEAT_TGran64K（第一级支持 64KB 内存转换粒度）
- FEAT_TIDCP1（EL0 使用由实现定义的功能）
- FEAT_TLBIOS（外部可共享域中的 TLB 失效指令）
- FEAT_TLBIRANGE（TLB 范围失效指令）
- FEAT_TTCNP（转换表公共而非私有的地址转换）
- FEAT_TTL（转换表级别）
- FEAT_TTST（小型转换表）
- FEAT_UAO（非特权访问覆盖控制）
- FEAT_VHE（虚拟化主机扩展）
- FEAT_VMID16（16 位 VMID）
- FEAT_WFxT（带超时的 WFE 和 WFI 指令）
- FEAT_XNX（转换表第二级非特权禁止执行）
- FEAT_XS（XS 属性）

有关这些扩展的具体信息，请参阅 `Arm A-profile 架构参考手册
<https://developer.arm.com/documentation/ddi0487/latest>`_。

模拟指定名称的 CPU 时，仅模拟该 CPU 硬件中存在的功能。（如果某项功能未列在上面，即使真实硬件应当具备它，也不受支持。） ``max`` CPU 启用所有功能。

R-profile CPU 架构支持
==================================

QEMU 的 TCG 对 R-profile CPU 的模拟支持目前有限，仅模拟 Cortex-R5 和 Cortex-R5F CPU。

M-profile CPU 架构支持
==================================

QEMU 的 TCG 模拟支持 M-profile 架构的 Armv6-M、Armv7-M、Armv8-M 和 Armv8.1-M 版本，还支持以下架构扩展：

- FP（浮点扩展）
- FPCXT（FPCXT 访问指令）
- HP（半精度浮点指令）
- LOB（低开销循环和未来分支）
- M（主扩展）
- MPU（内存保护单元扩展）
- PXN（特权禁止执行）
- RAS（可靠性、可维护性和可用性）：仅支持“最小 RAS 扩展”
- S（安全扩展）
- ST（系统定时器扩展）

有关这些扩展的具体信息，请参阅 `Armv8-M Arm 架构参考手册
<https://developer.arm.com/documentation/ddi0553/latest>`_。

模拟指定名称的 CPU 时，仅模拟该 CPU 硬件中存在的功能。（如果某项功能未列在上面，即使真实硬件应当具备它，也不受支持。）M-profile 没有与 ``max`` CPU 对应的模型。
