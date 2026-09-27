========
最佳实践
========

调试
========

可以通过 ``scripts/analyze-migration.py`` 分析迁移数据流。

用法示例：

.. code-block:: shell

  $ qemu-system-x86_64 -display none -monitor stdio
  (qemu) migrate "exec:cat > mig"
  (qemu) q
  $ ./scripts/analyze-migration.py -f mig
  {
    "ram (3)": {
        "section sizes": {
            "pc.ram": "0x0000000008000000",
  ...

更多选项见 ``analyze-migration.py -h``。

固件
========

迁移会传输 RAM 和 ROM 的副本，因此目标端运行时包含源端固件。
即使重置虚拟机，仍使用旧固件。只有重启 QEMU 后才会使用新固件。

- 固件大小变化可能改变保存固件所需的 RAMBlock 大小，导致迁移失败。
  实际使用中，最好将固件镜像填充到合适的 2 的幂次大小，并预留充分增长空间。

- 编写设备模拟代码时，需确保新版模拟代码兼容旧固件，以支持向新版本迁移。

- 更新固件时，需确保能够向使用旧设备模拟代码的旧系统反向迁移。

某些情况下，最好将特定固件版本绑定到特定的版本化机器类型，
减少需要支持的组合。新版固件超出预留填充空间时，这也很有用。
