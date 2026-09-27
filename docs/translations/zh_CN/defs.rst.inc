..
   通用的 RST 替换定义。docs/conf.py 将本文件加入所有文档的 epilogue，
   并另外定义 CONFDIR 等根据构建配置生成的替换项。

   qemu_system 和 qemu_system_x86 用于 parsed-literal 块。
   定义中不能额外加双反引号：HTML 可以正常显示，但 manpage 中后续普通文字
   会错误地继续显示为粗体。

.. |qemu_system| replace:: qemu-system-x86_64
.. |qemu_system_x86| replace:: qemu-system-x86_64
.. |I2C| replace:: I\ :sup:`2`\ C
.. |I2S| replace:: I\ :sup:`2`\ S
