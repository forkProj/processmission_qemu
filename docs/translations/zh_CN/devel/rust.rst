.. |msrv| replace:: 1.83.0

QEMU 中的 Rust
==============

Rust in QEMU 项目旨在允许用 Rust 为 QEMU 添加新功能。

当前重点是使用 `安全 Rust`__ 编写继承 SysBusDevice 的设备。
未来可能支持其他设备，例如可执行 DMA 的 PCI 设备，
以及完整开发板或块设备格式等后端。

__ https://doc.rust-lang.org/nomicon/meet-safe-and-unsafe.html

构建 QEMU Rust 代码
-------------------

Rust 代码通过 Meson 纳入模拟器。
Meson 直接调用 rustc 生成静态库，再与 C 代码链接。
运行 make 或 ninja 时自动完成。

构建系统也尽量照顾习惯普通 Cargo 工作流的开发者：

* 警告和 lint 配置始终来自工作区的 rust/Cargo.toml。

* 可用 cargo 执行 Rust 专用任务，尤其 clippy、rustfmt 和 rustdoc。

为此，build.rs 从 QEMU 构建目录获取生成源码，放入 Cargo 输出目录，通常为 rust/target/。
直接运行 Cargo 可能报告找不到生成源码，可通过以下方式解决：

* 使用 Meson 提供的 Makefile 目标运行 clippy 或 rustdoc：

    make clippy
    make rustdoc

rust/meson.build 还声明 rustfmt 目标：

    make rustfmt

* 通过 Meson 的 `开发环境`__ 运行 cargo::

    pyvenv/bin/meson devenv -w ../rust cargo clippy --tests
    pyvenv/bin/meson devenv -w ../rust cargo fmt


  若需反复使用，运行 pyvenv/bin/meson devenv 进入 shell，之后 cargo fmt 等即可直接使用。

__ https://mesonbuild.com/Commands.html#devenv

* 将 MESON_BUILD_ROOT 指向 QEMU 构建树顶层。
  使用 rust-analyzer 时很方便，可通过 rust-analyzer.cargo.extraEnv 设置。

如上所示，可正常用 --tests 操作测试代码。
但不能通过 cargo 构建或运行测试，因为它们依赖 Cargo 不知道的 QEMU C 支持代码。
应通过 Meson 的 pyvenv/bin/meson test，或 make 运行::

   make check-rust


doctest 要求构建生成的全部 .o 文件可用。

支持的工具
'''''''''''''''

QEMU 支持 rustc 1.83.0 及更新版本。
出于历史原因，以下较新功能尚未使用，欢迎补丁：

* 关联常量仍显式标记 'static，相关行为在 `1.81.0 中改变`__。

* &raw，在 1.82.0 稳定。

* track_caller 的 NUL 结尾文件名计划作为 location_file_nul 功能加入，
  QEMU 暂时还无法使用。目前 util/error.c 有专门代码处理非 NUL 结尾文件名。

部分 callbacks::FnCall 用途会受益于关联常量相等约束，
但它仍是实验性功能，目前改用常量断言。

__ https://github.com/rust-lang/rust/pull/125258

还支持 bindgen 0.60.x，它缺少 --generate-cstr。
该选项需要 0.66.x，不再需要支持旧版本后会采用。

在 QEMU 中编写 Rust
-------------------

QEMU 包含多个 crate：

* common：纯 Rust 工具。

* bql、chardev、hw/core、migration、qom、system、util：对应 QEMU C API 的绑定。

* qemu_macros：定义编写 C 代码时有用的多个过程宏。

* rust/hw/char/pl011 和 rust/hw/timer/hpet 中的 pl011、hpet：
  演示绑定和宏用法，并推动它们继续完善。
  它们分别是 hw/char/pl011.c 和 hw/timer/hpet.c 的可运行替代实现 [#issues]_。

.. [#issues] pl011 与提交 3e0f118f82 时的 C 版本同步，
   hpet 与提交 1433e38cc8 同步。两者均缺少跟踪功能。

以下介绍如何使用这些模块。

状态
''''''

模块稳定性分为：

- 完整：可用于新设备；如适用，API 支持 C 中的全部功能。
- 稳定：可用于生产，API 安全，预计不会有重大变化。
- 概念验证：API 可能变化，但可用安全 Rust 操作。
- 初始：仍需大量 unsafe 代码，可能存在健全性或类型安全问题。

当前状态如下：

========================== ======================
模块                       状态
========================== ======================
``bql::cell``              稳定
``common::assertions``     稳定
``common::bitops``         完整
``common::callbacks``      完整
``common::errno``          完整
``common::zeroable``       稳定
``hwcore::irq``            完整
``hwcore::qdev``           稳定
``hwcore::sysbus``         稳定
``migration::migratable``  概念验证
``migration::vmstate``     稳定
``qom``                    稳定
``system::memory``         稳定
``util::error``            稳定
``util::log``              概念验证
``util::module``           完整
``util::timer``            稳定
========================== ======================

.. note::
  API 稳定性不是承诺，毕竟对应 C API 也不是稳定接口。
  unsafe 接口未来也可能被安全接口替代。

命名约定
'''''''''''''''''

C 函数通常带数据类型前缀，例如 timer_mod、sysbus_connect_irq，
函数或结构有时还带 ``qemu_`` 或 QEMU。
Rust 中通常移除这些前缀：QEMUTimer 变为 timer::Timer，
timer_mod 变为 Timer::modify，sysbus_connect_irq 变为 SysBusDeviceMethods::connect_irq。

但同一 QOM 层级可能有仅前缀不同的同名操作，例如 qdev_realize 和 sysbus_realize。
名称不唯一时，较低层类始终保留前缀，最高层则按具体情况决定。例如：

========================== =========================================
``device_cold_reset()``    ``DeviceMethods::cold_reset()``
``pci_device_reset()``     ``PciDeviceMethods::pci_device_reset()``
``pci_bridge_reset()``     ``PciBridgeMethods::pci_bridge_reset()``
========================== =========================================

虽然名称不完全相同，PciDeviceMethods 仍加前缀避免混淆，
因为 device_cold_reset() 与 pci_device_reset() 功能略有区别。

以下情况最高层不需要前缀：

========================== =========================================
``device_realize()``       ``DeviceMethods::realize()``
``sysbus_realize()``       ``SysbusDeviceMethods::sysbus_realize()``
``pci_realize()``          ``PciDeviceMethods::pci_realize()``
========================== =========================================

较低层没有增加功能，主要提供额外编译时检查，
基本 realize 行为对所有设备相同，因此 DeviceMethods 不加前缀。

层级中名称唯一时，应始终去掉类名前缀。

常见陷阱
'''''''''''''''

Rust 对如何取得独占引用 &mut 有严格规则，违反会导致未定义行为。
即使值从原始可变指针 ``*mut`` 读出，也只有此前该指针来自可变引用时才能转为 &mut。
如果在写入原始指针和读取之间创建过共享引用，同样是未定义行为::

    let mut p: u32 = 42;
    let p_mut = &mut p;                              // 1
    let p_raw = p_mut as *mut u32;                   // 2

    // p_raw keeps the mutable reference "alive"

    let p_shared = &p;                               // 3
    println!("access from &u32: {}", *p_shared);

    // Bring back the mutable reference, its lifetime overlaps
    // with that of a shared reference.
    let p_mut = unsafe { &mut *p_raw };              // 4
    println!("access from &mut 32: {}", *p_mut);

    println!("access from &u32: {}", *p_shared);     // 5


可用 `MIRI`__ 等工具检查这些规则。

__ https://github.com/rust-lang/miri

QEMU Rust 几乎都会涉及 QOM 对象，而它们的指针是共享的，
例如对象属于 QOM 组合树，恰好形成上述情形：

1. 创建 QOM 对象。
2. 创建原始可变指针，例如作为 MemoryRegion 的 opaque。
3. 对象放入组合树。
4. 内存访问把 opaque 解引用为 &mut。
5. 但组合树中仍有共享引用。

因此 QOM 对象几乎总应使用 &self，而非 &mut self，
并通过内部可变性从共享引用访问可变内部字段。

C 代码提供不透明 void 指针时，不要转为 Rust 可变引用，应使用共享引用。
bql::cell 封装向编译器表达内部可变性，也可强制 BQL 加锁规则。
未来可能为 AioContext 锁提供类似 cell。

设备代码通常使用 BqlRefCell 和 BqlCell，确保数据访问受 BQL 正确保护。
vmstate 也认识这些 cell，可在建立结构内存布局时查看其内部。
RefCell 和 Mutex 则不具备这种支持。

绑定代码通常使用 Opaque，隐藏底层结构，并便于转换为调用 C 函数所需的原始指针。例如::

    #[repr(transparent)]
    #[derive(Debug, common::Wrapper)]
    pub struct Object(Opaque<bindings::Object>);


专用 derive 宏提供 from_raw、as_ptr、as_mut_ptr、raw_get 等方法。
绑定通过断言手动检查 BQL，因而可以将封装声明为线程安全::

    unsafe impl Send for Object {}
    unsafe impl Sync for Object {}


编写 C 代码绑定
''''''''''''''''''''''''''

开发 QEMU Rust crate 时应注意：

**参考已有代码**
  类似 C 模式通常对应类似 Rust 技巧。
  C 使用 offsetof 时参考 qdev 属性或 vmstate；复杂常量结构可参考 MemoryRegion。
  复用生命周期处理模式：无需引用计数的 QOM 对象，包括嵌入其他对象的对象，
  使用 &T；需要计数时用 Owned<T>。

**利用类型系统**
  绑定常需要取得内置或用户类型的专属信息并传给 C。
  把这些信息放入 trait，通过泛型参数访问。
  vmstate 提供获取 Rust 结构字段类型信息的示例。

**优先 unsafe trait，而非 unsafe 函数**
  trait 更易证明正确，适合保存供泛型函数读取的元数据。
  C 常将元数据放在全局变量，Rust 可先放 trait，再生成 static 变量。
  unsafe trait 常可由过程宏生成。

**记录旧 Rust 版本造成的限制**
  若因受支持版本而采用较差方案，应在源码和本文记录，
  以便最低版本提高后修正。

**牢记加锁要求**
  声明 Sync 时，注意是否需要 BQL。
  内部数据使用 BqlCell、BqlRefCell，或断言 bql_locked()。

**可以使用复杂技巧，但要隔离并记录**
  设备代码比绑定代码更常编写，应使其符合 Rust 惯用方式。
  技巧应集中到可文档化、可测试的结构、trait 或宏中，
  必要时在文档提供简化示例。

FFI 绑定生成
''''''''''''''''''''''

Rust 集成使用多个 ``*-sys`` crate，保存各子系统的原始 FFI 绑定。
它们对应 meson.build 通过 static_library() 表达的 C 依赖结构。例如：

* util-sys：基础工具，无依赖。
* qom-sys：对象模型，依赖 util-sys。
* chardev-sys：字符设备，依赖 qom-sys、util-sys。
* hwcore-sys：硬件核心，依赖 qom-sys、util-sys。
* migration-sys：迁移，依赖 util-sys。
* system-sys：系统级 API，依赖其余全部。

多个 crate 避免 C 头文件变化导致全部 Rust 大规模重建。
但 bindgen 不知道头文件如何跨 crate 划分，会重复生成依赖声明。
这些声明不但庞大，还产生彼此不同、互不兼容的类型。

Bindgen 配置
~~~~~~~~~~~~

符号阻止列表、枚举处理等选项，可在各 Cargo.toml 的
[package.metadata.bindgen] 中配置。例如::

    [package.metadata.bindgen]
    header = "wrapper.h"                    # Main header file for this crate
    rustified-enum = ["QEMUClockType"]      # Enums to generate as Rust enums
    bitfield-enum = ["VMStateFlags"]        # Enums to treat as bitfields
    blocklist-function = [                  # Functions to exclude
        "vmstate_register_ram",
        "vmstate_unregister_ram"
    ]
    additional-files = [                    # Extra files to allowlist
        "include/system/memory_ldst.*"
    ]


支持全部 bindgen 选项，完整列表见 rust/bindings/generate_bindgen_args.py。

依赖管理
~~~~~~~~

在 bindgen 生成代码前检查依赖链，
确保一个 crate 已包含的头文件不会再次出现在依赖它的 crate 中，
避免重复定义。依赖方通过 use 导入。

rust/bindings/generate_bindgen_args.py 自动按依赖顺序处理 Cargo.toml，
为 bindgen 生成适当 --allowlist-file 和 --blocklist-file 参数。

编写过程宏
'''''''''''''''''''''''''

惯例上，过程宏分为两个函数：
实现函数返回 ``Result<proc_macro2::TokenStream, syn::Error>``，
真正的过程宏返回 ``proc_macro::TokenStream``。
前者名称是后者加 _or_error 后缀。
后者代码基本固定，除 parse_macro_input! 中 as 后类型外，遵循以下模板::

    #[proc_macro_derive(Object)]
    pub fn derive_object(input: TokenStream) -> TokenStream {
        let input = parse_macro_input!(input as DeriveInput);

        derive_object_or_error(input)
            .unwrap_or_else(syn::Error::into_compile_error)
            .into()
    }


qemu_macros 提供检查 DeriveInput 的辅助函数，
例如确认结构具有命名字段，返回 Result<..., syn::Error>，便于组合::

    fn derive_object_or_error(input: DeriveInput) ->
        Result<proc_macro2::TokenStream, Error>
    {
        is_c_repr(&input, "#[derive(Object)]")?;

        let name = &input.ident;
        let parent = &get_fields(&input, "#[derive(Object)]")?[0].ident;
        ...
    }


过程宏应谨慎使用，主要适合：

* 一致性检查，例如 derive(Object) 检查 repr(C)，
  以及首字段类型是否与 ObjectType 声明一致。

* 将源码中的类型和属性信息提取到 trait。
  例如 derive(TryInto) 构建 TryFrom 实现，并用 repr 属性确定源类型和错误类型。

过程宏难以调试测试。如果生成代码超过几行，
可考虑将工作委托给普通 macro_rules! 声明宏，并为后者编写单元测试。


编码风格
''''''''''''

代码应通过 clippy，并用 rustfmt 格式化。
目前仅支持 nightly rustfmt，未来可能改变。
CI 通过 cargo fmt --check 检查，维护者也可以在应用补丁时代为修正格式。

已定型或接近定型的绑定，应有完整 rustdoc 文档。

添加依赖
--------

通常保持依赖 crate 集合精简。
添加外部 crate 前应慎重，尤其其自身带来大量依赖时。
有时 QEMU 只需要少量功能，例如 assertions 模块。

此外，添加 crate 流程稍复杂，主要需要让 Meson 知道如何构建。
Meson 虽初步支持 Cargo.lock，但仍高度实验性，当前未使用。

因此必须将外部 crate 同时加入 Meson 子项目及相关 Cargo.toml。
rust/Cargo.lock 版本必须与子项目一致。
rust/ 是 Cargo `工作区`__，整个构建共用一个锁文件。

__ https://doc.rust-lang.org/cargo/reference/workspaces.html#virtual-workspace

应选择兼容最低受支持 Rust 版本 |msrv| 的 crate 版本。

其次添加 wrap，告诉 Meson 如何下载，命名为 NAME-SEMVER-rs.wrap。
NAME 是 crate 名，SEMVER 保留版本号到首个非零数字。
例如 0.2.3 使用 0.2，1.0.84 使用 1。

然后在 subprojects/NAME-SEMVER-rs/meson.build 添加规则，通常包括：

* 各依赖的 subproject 和 dependency；
* 执行构建的 static_library 或 rust.proc_macro；
* 向 QEMU 和其他子项目暴露结果的 declare_dependency 和 meson.override_dependency。

过程宏的依赖需为 dependency、static_library、meson.override_dependency
添加 native: true。
同一 crate 同时供过程宏和 QEMU 二进制使用时，除 subproject 外其余定义都需复制，
分别构建本机与非本机版本。

必须指定正确编译选项，包括：

* Cargo.toml 中的语言 edition；
* 从 crate 的 build.rs 分析得到的 --cfg；
* 通常还需要 --cap-lints allow，隐藏 rustc 或 clippy 警告。

每次修改 meson.build 后，都需用 ``meson subprojects update --reset NAME-SEMVER-rs``
更新应用补丁后的版本，未来可能自动化。

还强烈建议同时对 .wrap 作无实际影响的修改，例如添加 # version 2 注释，
帮助 Meson 发现子项目已过期。

最后，将子项目加入 scripts/archive-source.sh、scripts/make-release 和 subprojects/.gitignore。
