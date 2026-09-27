指定跟踪选项。

``[enable=]PATTERN``

  立即启用匹配 *PATTERN* 的事件，匹配对象可以是事件名或通配符模式。
  仅当 QEMU 编译时启用了 ``simple``、``log`` 或 ``ftrace`` 跟踪后端时，此选项才可用。
  若要指定多个事件或模式，可以重复使用 :option:`-trace`。

  使用 :option:`-trace help` 打印跟踪点名称列表。

``events=FILE``

  立即启用 *FILE* 中列出的事件。
  文件每行应包含一个事件名，与 ``trace-events-all`` 中的名称对应，也接受通配符模式。
  仅当 QEMU 编译时启用了 ``simple``、``log`` 或 ``ftrace`` 跟踪后端时，此选项才可用。

``file=FILE``

  将跟踪日志输出到 *FILE*。
  仅当 QEMU 编译时启用了 ``simple`` 跟踪后端时，此选项才可用。
