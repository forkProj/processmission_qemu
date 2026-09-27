模拟期间，如果使用字符后端多路复用器（``-nographic`` 的默认行为），
可以通过转义序列执行若干命令。它们均以转义字符开头，默认 :kbd:`Ctrl+a`，
可通过 ``-echr`` 修改。下表假定使用默认值。

.. list-table:: 多路复用器按键
  :widths: 20 80
  :header-rows: 1

  * - 按键序列
    - 操作

  * - :kbd:`Ctrl+a h`
    - 输出此帮助

  * - :kbd:`Ctrl+a x`
    - 退出模拟器

  * - :kbd:`Ctrl+a s`
    - 将磁盘数据写回文件（使用 -snapshot 时）

  * - :kbd:`Ctrl+a t`
    - 切换控制台时间戳

  * - :kbd:`Ctrl+a b`
    - 发送 break（Linux 中为 magic sysrq）

  * - :kbd:`Ctrl+a c`
    - 轮换连接到多路复用器的前端，通常是在监视器和控制台之间切换

  * - :kbd:`Ctrl+a Ctrl+a`
    - 向前端发送转义字符
