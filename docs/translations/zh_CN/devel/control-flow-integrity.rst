.. _cfi:

===================
控制流完整性（CFI）
===================

本文介绍 QEMU 当前的控制流完整性机制，包括启用方式、优缺点，
以及它对新代码和已有代码的影响。

基础
--------

CFI 是一种加固技术，重点保证间接函数调用未被攻击者篡改。
QEMU 使用前向边控制流完整性，确保通过函数指针调用时始终进入“兼容”函数。
兼容指函数签名与源码中声明的函数指针签名相同。

这类 CFI 完全由编译器实现，依赖编译器知道所有函数和函数指针的签名。
目前只有 Clang 提供支持。

CFI 最适合用于生产二进制文件，防御未知攻击途径。
发生违规，即调用不兼容函数时，QEMU 会立即终止，阻断潜在攻击。

启用 CFI 构建
-------------

注意：CFI 要求链接时优化，因此选择 CFI 后会自动启用 LTO。

最低需要 Clang 6+。如果还要启用模糊测试，则需要 Clang 11+，详见后文。

由于使用 LTO，归档工具 AR 必须支持 LLVM IR。
最简单的方式是选用 LLVM 提供的 AR::

 AR=llvm-ar-9 CC=clang-9 CXX=clang++-9 /path/to/configure --enable-cfi


所有生成的二进制文件都会启用 CFI。

还可使用 ``--enable-debug-cfi``，在违规时输出更详细信息。

使用启用 CFI 的 QEMU
--------------------

其使用方式与普通二进制文件完全相同。
发生 CFI 违规时，程序会因非法指令信号终止。

不兼容 CFI 的代码
-----------------

CFI 完全基于编译器，依赖编译时对代码的了解。
虽然大多数代码均支持，但某些模式会破坏兼容性并产生误报，主要有：

* 即时编译代码：运行时才生成，因此跳转到 JIT 代码缓冲区会被检查阻止。

* 动态加载的库，例如通过 dlopen/dlsym，因为编译时不知道该库。

QEMU 当前未完全兼容的领域包括：

1. TCG：运行时提前编译指令组以加快解释，类似 JIT。
2. TCI：解释器需要解释通用 call 操作。
3. 插件：以外部库实现。
4. 模块：以外部库实现。
5. 从 QEMU 源码直接调用信号处理器：处理器可能来自外部库，甚至在运行时才接入。

为特定函数禁用 CFI
------------------

如果函数采用上述不兼容方式调用，可在定义处添加 ``QEMU_DISABLE_CFI``，
选择性禁用该函数的检查，并解释不兼容原因。例如::

	/*
	 * Disable CFI checks.
	 * TCG creates binary blobs at runtime, with the transformed code.
	 * A TB is a blob of binary code, created at runtime and called with an
	 * indirect function call. Since such function did not exist at compile time,
	 * the CFI runtime has no way to verify its signature and would fail.
	 * TCG is not considered a security-sensitive part of QEMU so this does not
	 * affect the impact of CFI in environment with high security requirements
	 */
	QEMU_DISABLE_CFI
	static inline tcg_target_ulong cpu_tb_exec(CPUState *cpu, TranslationBlock *itb)


注意：必须在 **调用者** 函数禁用 CFI，
也就是调用不兼容函数的那个兼容函数，因为检查发生在调用执行时。

CFI 与模糊测试
--------------

两者面向不同环境：CFI 用于生产，模糊测试用于调试，通常联合使用没有优势。

CFI 可以帮助模糊测试发现不会立即触发段错误或断言的更多缺陷，
但地址和未定义行为检测器等工具能更精确地识别它们。

一个有价值的组合场景是，确认代码中罕见但仍可能执行的路径不会触发 CFI 误报。

同时启用时有以下限制：

1. 模糊测试依赖链接时函数包装。标准 BFD 链接器在 LTO 启用时不支持该功能，
   可改用 LLVM 的 lld。
2. 模糊测试还依赖自定义链接脚本，lld 从 11+ 才支持。

因此需要 clang 11+，并使用 lld 链接::

 AR=llvm-ar-11 CC=clang-11 CXX=clang++-11 /path/to/configure --enable-cfi \
                           -enable-fuzzing --extra-ldflags="-fuse-ld=lld"


然后按通常方式编译模糊测试程序。
