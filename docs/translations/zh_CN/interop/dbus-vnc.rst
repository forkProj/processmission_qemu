D-Bus VNC
========================

独立 VNC 服务器 ``qemu-vnc`` 提供 D-Bus 接口，用于管理和监视 VNC 连接。

服务使用总线上的知名名称 ``org.qemu.vnc``，对象导出在 ``/org/qemu/Vnc1/`` 下。

.. contents::
   :local:
   :depth: 1

.. only:: sphinx4

   .. dbus-doc:: tools/qemu-vnc/qemu-vnc1.xml

.. only:: not sphinx4

   .. warning::
      构建 D-Bus 文档需要 Sphinx 4。

      以下为 ``tools/qemu-vnc/qemu-vnc1.xml`` 的内容：

   .. literalinclude:: ../../tools/qemu-vnc/qemu-vnc1.xml
      :language: xml
