
.. _setup-build-env:

设置构建环境
============

QEMU 依赖主机上的许多组件，其中大量是可选的。
至少需要系统 C 库（通常为 glibc，也可使用其他实现）、
代码中广泛使用的 glib2，以及若干用于对接代码模块和系统构建描述的核心库。

我们使用 libvirt-ci 将依赖映射到多种输出格式，包括系统安装脚本。例如：

.. code-block:: bash

  # THIS FILE WAS AUTO-GENERATED
  #
  #  $ lcitool buildenvscript debian-13 ./tests/lcitool/projects/qemu-minimal.yml
  #
  # https://gitlab.com/libvirt/libvirt-ci

  function install_buildenv() {
      export DEBIAN_FRONTEND=noninteractive
      apt-get update
      apt-get dist-upgrade -y
      apt-get install --no-install-recommends -y \
              bash \
              bc \
              bison \
              bzip2 \
              ca-certificates \
              ccache \
              findutils \
              flex \
              gcc \
              git \
              libc6-dev \
              libfdt-dev \
              libffi-dev \
              libglib2.0-dev \
              libpixman-1-dev \
              locales \
              make \
              meson \
              ninja-build \
              pkgconf \
              python3 \
              python3-venv \
              sed \
              tar
      sed -Ei 's,^# (en_US\.UTF-8 .*)$,\1,' /etc/locale.gen
      dpkg-reconfigure locales
      rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED
      dpkg-query --showformat '${Package}_${Version}_${Architecture}\n' --show > /packages.txt
      mkdir -p /usr/libexec/ccache-wrappers
      ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
      ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
  }

  export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
  export LANG="en_US.UTF-8"
  export MAKE="/usr/bin/make"
  export NINJA="/usr/bin/ninja"
  export PYTHON="/usr/bin/python3"

如果改选 qemu.yml 项目文件，就会获得项目可使用的全部依赖。

使用系统软件包管理器
--------------------

.. note::

   使用软件包管理器的构建依赖功能，可能遗漏 qemu.git 新引入的依赖，
   也会遗漏发行版选择排除的依赖。

具有软件包管理器的系统
++++++++++++++++++++++

.. list-table:: 软件包管理命令
  :widths: 10 50 40
  :header-rows: 1

  * - 系统
    - 命令
    - 说明
  * - Fedora
    - ``sudo dnf update && sudo dnf builddep qemu``
    -
  * - Debian/Ubuntu
    - ``sudo apt update && sudo apt build-dep qemu``
    - 必须先启用 `源码源列表 <https://wiki.debian.org/SourcesList>`_
  * - MacOS
    - ``brew update && brew install $(brew deps --include-build qemu)``
    - 使用 `Homebrew <https://brew.sh/>`_。

Windows
++++++++

首先安装 `MSYS2 <https://www.msys2.org/>`_。
它提供 `多种环境 <https://www.msys2.org/docs/environments/>`_，
x86_64 基于 GCC，aarch64 基于 Clang。

建议 windows-x86_64 使用 MINGW64，windows-aarch64 使用 CLANGARM64，
后者仅在 windows-aarch64 主机上可用。

随后打开 Windows shell，按如下方式进入 MSYS2 环境：

::

    c:/msys64/msys2_shell.cmd -defterm -here -no-start -mingw64
    # Replace -ucrt64 by -clangarm64 or -ucrt64 for other environments.

MSYS2 包管理器没有内置的构建依赖安装功能。
可以先用 pacman 安装以下软件包：

注意：切换到不同 MSYS2 环境后，需要重新安装依赖。

::

    # update MSYS2 itself, you need to reopen your shell at the end.
    pacman -Syu
    pacman -S \
        base-devel binutils bison diffutils flex git grep make sed \
        ${MINGW_PACKAGE_PREFIX}-toolchain \
        ${MINGW_PACKAGE_PREFIX}-glib2 \
        ${MINGW_PACKAGE_PREFIX}-gtk3 \
        ${MINGW_PACKAGE_PREFIX}-libnfs \
        ${MINGW_PACKAGE_PREFIX}-libssh \
        ${MINGW_PACKAGE_PREFIX}-ninja \
        ${MINGW_PACKAGE_PREFIX}-pixman \
        ${MINGW_PACKAGE_PREFIX}-pkgconf \
        ${MINGW_PACKAGE_PREFIX}-python \
        ${MINGW_PACKAGE_PREFIX}-SDL2 \
        ${MINGW_PACKAGE_PREFIX}-zstd

若要安装全部依赖，可以使用 MSYS2 自身构建 QEMU 的配方。

::

    pacman -S wget base-devel git
    wget https://raw.githubusercontent.com/msys2/MINGW-packages/refs/heads/master/mingw-w64-qemu/PKGBUILD
    # Some packages may be missing for your environment, installation will still
    # be done though.
    makepkg --syncdeps --nobuild PKGBUILD || true

在 windows-aarch64 上构建
~~~~~~~~~~~~~~~~~~~~~~~~~

在 UCRT64 或 MINGW64 环境尝试通过 Meson 为 x86_64 交叉编译时，
configure 会因检测到的 CPU 不正确而报错。

Meson 检测到的是模拟运行的 x86_64 进程，
因此需手动设置 CPU，并以空前缀强制交叉编译。

::

    ./configure --cpu=x86_64 --cross-prefix=
