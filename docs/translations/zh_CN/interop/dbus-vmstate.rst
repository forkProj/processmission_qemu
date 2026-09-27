========================
D-Bus VMState
========================

QEMU 的 dbus-vmstate 对象用于迁移运行在 QEMU D-Bus 总线上的辅助程序数据。
D-Bus 使用建议见 :doc:`dbus`。

迁移时，QEMU 遍历 ``org.qemu.VMState1`` D-Bus 名称所有者队列，查询各自的 ``Id``。
辅助程序之间的 Id 必须唯一。

随后保存每个 Id 的任意数据，通过迁移流传输，并在目标端对应辅助程序中恢复或加载。

目前传输数据量暂定限制为 1Mb。状态必须快速保存，在不到一秒内完成。
D-Bus 本身也限制回复时间；若数据提供不够快，迁移会失败。

可通过 dbus-vmstate 的 ``id-list`` 属性配置预期辅助程序列表，
值为逗号分隔的 ``Id`` 列表。

.. only:: sphinx4

   .. dbus-doc:: backends/dbus-vmstate1.xml

.. only:: not sphinx4

   .. warning::
      构建 D-Bus 文档需要 Sphinx 4。

      以下为 ``backends/dbus-vmstate1.xml`` 的内容：

   .. literalinclude:: ../../backends/dbus-vmstate1.xml
      :language: xml
