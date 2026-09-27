.. _dbus-display:

D-Bus 显示
========================

QEMU 以 ``-display dbus`` 启动时，可通过 D-Bus 导出虚拟机显示，
供进程外界面、远程协议服务器或其他交互显示用途使用。

根据虚拟机配置，``/org/qemu/Display1/`` 下不同对象路径提供多种专用 D-Bus 接口。

QEMU 也实现标准接口，例如
`org.freedesktop.DBus.Introspectable
<https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces>`_。

.. contents::
   :local:
   :depth: 1

.. only:: sphinx4

   .. dbus-doc:: ui/dbus-display1.xml

.. only:: not sphinx4

   .. warning::
      构建 D-Bus 文档需要 Sphinx 4。

      以下为 ``ui/dbus-display1.xml`` 的内容：

   .. literalinclude:: ../../ui/dbus-display1.xml
      :language: xml
