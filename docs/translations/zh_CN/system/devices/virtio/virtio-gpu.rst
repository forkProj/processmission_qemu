..
   SPDX-License-Identifier: GPL-2.0-or-later

.. _virtio_gpu:

VirtIO GPU
==========

本文介绍 virtio-gpu 的设置和使用。它通过 VirtIO 提供半虚拟化 GPU 和显示
控制器，支持从简单二维显示到完整加速三维图形的多种模式。

Linux 客户机内核支持
--------------------------

客户机内核必须启用 CONFIG_DRM_VIRTIO_GPU。

三维加速
---------------

虚拟 GPU 三维加速仍在发展。根据所用模式，可能需要以更新版本替换发行版库，
或启用特定构建选项。主机必须满足若干要求：QEMU 能访问 GPU，且为获得最佳
性能，能够可靠地与客户机共享 GPU 内存。详细要求见后文。

QEMU virtio-gpu 变体
------------------------

变体形式为：

 * ``virtio-vga[-BACKEND]``
 * ``virtio-gpu[-BACKEND][-INTERFACE]``
 * ``vhost-user-vga``
 * ``vhost-user-pci``

**后端：** 提供默认二维后端，以及 virglrenderer（gl）和 rutabaga_gfx
（rutabaga）两个加速后端。vhost-user 后端可将图形栈放在独立进程，提高隔离。

**接口：** 根据客户机接口分为 VGA 和非 VGA。前者以前缀 virtio-vga 或
vhost-user-vga 表示，后者为 virtio-gpu 或 vhost-user-gpu。

VGA 始终使用 PCI。非 VGA 可选 MMIO 或 PCI：-device 后缀表示 MMIO，但
vhost-user-gpu 不支持；-pci 表示 PCI。无后缀则选择平台默认接口。

virtio-gpu 二维后端
-------------------

默认后端只执行二维操作，三维需客户机软件渲染器。

通常使用 `Mesa`_ 或 `SwiftShader`_。Mesa 的 LLVMpipe、Lavapipe 及下文的
virgl 在典型现代 Linux 发行版上可直接使用。

.. parsed-literal::
    -device virtio-gpu


.. _Mesa: https://www.mesa3d.org/
.. _SwiftShader: https://github.com/google/swiftshader

virtio-gpu virglrenderer
------------------------

客户机使用 `virgl`_ 加速时，OpenGL 调用转换为中间表示，见 `Gallium3D`_，
传到主机后由 `virglrenderer`_ 转回 OpenGL 调用。

客户机默认最高 OpenGL 4.3。启用 4.6 需开启 host blob，即 hostmem 和 blob。
hostmem 是主机内存窗口大小，通常 256M 到 8G。

.. parsed-literal::
    -device virtio-gpu-gl
    -device virtio-gpu-gl,hostmem=8G,blob=true


.. _virgl: https://docs.mesa3d.org/drivers/virgl.html
.. _Gallium3D: https://www.freedesktop.org/wiki/Software/gallium/
.. _virglrenderer: https://gitlab.freedesktop.org/virgl/virglrenderer/

virglrenderer 1.0.0 起通过 `venus`_ 支持 Vulkan 转换。Venus capset 要求
hostmem、blob，并通过 venus 字段启用。主机窗口通常为 256M 到 8G。

.. parsed-literal::
    -device virtio-gpu-gl,hostmem=8G,blob=true,venus=true


.. _venus: https://docs.mesa3d.org/drivers/venus.html

同版本起通过 `drm`_ 协议支持 DRM 原生上下文。DRM capset 同样要求 hostmem、
blob，通过 drm_native_context 启用，窗口通常为 256M 到 8G。

.. parsed-literal::
    -device virtio-gpu-gl,hostmem=8G,blob=on,drm_native_context=on


.. _drm: https://gitlab.freedesktop.org/virgl/virglrenderer/-/tree/main/src/drm

.. list-table:: Linux 主机要求
  :header-rows: 1

  * - 功能
    - 内核版本
    - Libvirglrenderer 版本
  * - OpenGL 直通
    - 不用 host blob 时为任何兼容 QEMU 的 Linux，否则为 6.13+
    - 0.8.2+
  * - Vulkan 直通
    - Linux 6.13+
    - 1.0.0+
  * - AMDGPU DRM 原生上下文
    - Linux 6.13+
    - 1.1.0+
  * - Freedreno DRM 原生上下文
    - Linux 6.4+
    - 1.0.0+
  * - Intel i915 DRM 原生上下文
    - Linux 6.13+
    - 1.3.0+
  * - Asahi DRM 原生上下文
    - Asahi Linux 内核的 `Downstream version`_
    - 1.2.0+
  * - Panfrost 原生上下文
    - Linux 6.13+
    - 1.3.0+

.. _Downstream version: https://github.com/AsahiLinux/linux

.. list-table:: Linux 客户机要求
  :header-rows: 1

  * - 功能
    - 内核版本
    - Mesa 版本
  * - OpenGL 直通
    - 任何支持 virtio-gpu 的 Linux
    - 16.0.0+
  * - Vulkan 直通
    - Linux 5.16+
    - 24.2.0+
  * - AMDGPU DRM 原生上下文
    - Linux 6.14+
    - 25.0.0+
  * - Freedreno DRM 原生上下文
    - Linux 6.14+
    - 23.1.0+
  * - Intel i915 DRM 原生上下文
    - Linux 6.14+
    - 26.1.0+
  * - Asahi DRM 原生上下文
    - Linux 6.14+
    - 24.2.0+
  * - Panfrost 原生上下文
    - Linux 6.14+
    - `mr36814`_

.. _mr36814: https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/36814

virtio-gpu rutabaga
-------------------

rutabaga_gfx 提供 `gfxstream`_ 渲染和 `Wayland display passthrough`_。
gfxstream 以最少修改将 GLES 和 Vulkan 调用转发到主机。

crosvm 手册介绍如何构建 `gfxstream-enabled rutabaga`_ 和运行
`guest Wayland proxy`_。

此设备要求 host blob 支持，hostmem 指定主机窗口，通常 256M 到 8G。

启动时至少指定一个 capset。Linux 支持 gfxstream-vulkan 和 cross-domain；
Android 还支持实验性的 x-gfxstream-gles、x-gfxstream-composer。

启用 cross-domain 时自动探测 Wayland 套接字，非标准路径可用
wayland-socket-path 指定。

wsi 可为 surfaceless 或 headless。前者不创建原生窗口，但收到 virtio-gpu
二维 hypercall 时，会从渲染目标复制到 Pixman 缓冲；后者连这一步也不做。
未指定时默认 surfaceless。

.. parsed-literal::
    -device virtio-gpu-rutabaga,gfxstream-vulkan=on,cross-domain=on,
       hostmem=8G,wayland-socket-path=/tmp/nonstandard/mock_wayland.sock,
       wsi=headless


.. _gfxstream: https://android.googlesource.com/platform/hardware/google/gfxstream/
.. _Wayland display passthrough: https://www.youtube.com/watch?v=OZJiHMtIQ2M
.. _gfxstream-enabled rutabaga: https://crosvm.dev/book/appendix/rutabaga_gfx.html
.. _guest Wayland proxy: https://crosvm.dev/book/devices/wayland.html

.. list-table:: Linux 主机要求
  :header-rows: 1

  * - 功能
    - 内核版本
  * - Vulkan+Wayland 直通
    - Linux 6.13+

.. list-table:: Linux 客户机要求
  :header-rows: 1

  * - 功能
    - 内核版本
    - Mesa 版本
  * - Vulkan+Wayland 直通
    - Linux 5.16+
    - 24.3.0+
