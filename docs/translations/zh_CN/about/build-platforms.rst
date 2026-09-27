.. _Supported-build-platforms:

支持的构建平台
==============

QEMU 致力于支持在多种宿主操作系统上构建和运行。
本附录列出主要构建目标，项目据此决定第三方依赖的最低版本。
补丁提交审阅时，以及合入前后，项目会在这些平台上运行自动化测试。

未列出的平台并不意味着无法运行 QEMU。
如果软件版本与已列平台相近，通常应当能够工作。
对于未列平台的问题，也欢迎报告缺陷，除非该平台明显比这里描述的环境更老。

评估发行版所提供的软件版本时，QEMU 只考虑版本号，
并假定其功能与同版本上游发布一致。
如果发行版向旧版本回移了额外功能，QEMU 上游不会专门支持这些回移，
除非能够用同样适用于上游版本的方式自动检测该功能。

`Repology`_ 可帮助查询各操作系统目前提供的软件版本，
但并未覆盖下文所有发行版。

各系统的构建依赖安装方法见 :ref:`setup-build-env`。

支持的宿主架构
--------------

以下宿主架构得到正式支持，并可使用相应加速器：

  .. list-table::
   :header-rows: 1

   * - CPU 架构
     - 加速器
   * - Arm
     - hvf, kvm, tcg, whpx, xen
   * - PPC
     - kvm, tcg
   * - RISC-V
     - kvm, tcg
   * - s390x
     - kvm, tcg
   * - SPARC
     - tcg
   * - x86
     - hvf, mshv, kvm, nvmm, tcg, whpx, xen

其他宿主架构不受支持。可以通过 configure 的 ``--enable-tcg-interpreter``
启用 TCI，在不受支持的架构上构建 QEMU 系统模拟，但速度很慢，不建议用于正常使用。
QEMU 用户态模拟需要宿主专用的信号处理支持，因此 TCI 无法解决这类架构的用户态支持问题。

不受支持的架构未来可能按 :ref:`弃用流程 <Deprecated features>` 移除。

Linux、macOS、FreeBSD、NetBSD、OpenBSD
--------------------------------------

项目原则上始终支持最新主要版本，支持期最长为首次发布后的五年。
前一个主要版本的支持会在新主要版本发布两年后结束，
如果供应商更早终止支持，则以供应商期限为准。
这里不考虑第三方延长发行版生命周期的计划，即使得到供应商认可，
例如 Debian LTS；也不考虑从较新版本回移软件包的仓库，例如 Debian backports。
对于每个主要版本，只考虑最新的次要版本。

判断 Linux 可用软件版本时，项目参考 CentOS、Debian、Fedora、openSUSE、RHEL、
SLES 和 Ubuntu LTS，并假定其他发行版提供相近版本。

FreeBSD 和 OpenBSD 以各自 ports 仓库为依据，NetBSD 则以 pkgsrc 为依据。

macOS 使用 `Homebrew`_ 作为参考，同时预计 `MacPorts`_ 提供类似版本。

部分构建依赖可能遵循不那么保守的规则。

Python 运行时
  长期支持发行版通常提供多个 Python 运行时版本。
  QEMU 最初会尽量支持发行版默认版本，但之后可能将最低要求提升到供应商提供的
  其他较新可选版本。此时必须使用 ``configure`` 的 ``--python`` 选项，
  指向受支持的 Python 运行时。

  QEMU |version| 的最低 Python 版本为 3.9。

Python 构建依赖
  部分构建依赖由 Python 编写，发行版通常只为默认 Python 运行时打包它们。
  如果 QEMU 提高最低版本要求，需要使用非默认运行时，
  可能还需要通过 ``pip`` 从 Python Package Index（PyPI）安装相应模块。

Rust 构建依赖
  QEMU 对新增 Rust 依赖通常较为谨慎，并将这些依赖包含在发布压缩包中。
  bindgen 工具是例外，因为体积太大，不适合一同分发。
  最低支持 bindgen 0.60.x；若发行版未提供或版本过旧，
  建议通过 ``cargo install --locked bindgen-cli`` 安装较新版本。

  QEMU 需要 Rust 1.83.0。除 Debian bookworm 的 ``mips64el`` 架构外，
  所有受支持平台均可获得该版本。
  Debian bookworm 的其他架构可通过 ``rustc-web`` 获得足够新的编译器。

  Ubuntu 22.04（Jammy）和 24.04（Noble）可通过 ``rustc-1.83``
  等软件包获取更新版本，但必须向 configure 手动提供 ``rustc`` 和 ``rustdoc`` 路径。

  有些发行版不希望使用随源码附带的 crate，而偏好 ``/usr/share/cargo/registry``
  等位置的本地源码。QEMU 提供 ``scripts/get-wraps-from-cargo-registry.py``
  自动完成此配置，应在解压 QEMU 发布包后调用。
  还提供 ``rust/Cargo.toml`` 和 ``rust/Cargo.lock``，可用于计算构建依赖，
  例如运行 ``cargo2rpm -p rust/Cargo.toml buildrequires``。

可选构建依赖
  缺少某些组件并不影响 QEMU 本身的构建，而发行版可能未提供它们，或版本过旧。
  其中很多工具由 Python 编写，例如功能测试框架的额外模块和各种代码检查器，
  可以通过 ``pip`` 安装。
  交叉编译器也是可选构建依赖，可以从 EPEL 等仓库获取，
  使用 ``docker`` 或 ``podman`` 进行容器化交叉编译，
  或使用 QEMU 随附的预构建二进制文件。

Windows
--------

项目致力于支持仍由供应商维护的最新两个 Windows 版本。
当前目标最低 Windows API 为 Windows 8，因此理论上二进制文件也可以在较旧系统运行。
但这些旧版本不再接受测试，建议使用较新的 Windows。

项目支持使用当前 MinGW 工具链构建 QEMU，
可以在 Linux（Debian/Fedora）上交叉构建，也可以在 Windows 上使用 `MSYS2`_。
通常优先选择较新的 Windows，以减少 MSYS2 构建问题。
构建过程中有些 Python 脚本会调用 os.symlink()，需要特别注意符号链接权限。
较新的 Windows 10 开启开发人员模式后，非特权账户也能创建符号链接；
如果无法启用该模式，则需要 SeCreateSymbolicLinkPrivilege 权限，或以管理员身份运行。

只支持 64 位 Windows。

.. _Homebrew: https://brew.sh/
.. _MacPorts: https://www.macports.org/
.. _MSYS2: https://www.msys2.org/
.. _Repology: https://repology.org/
