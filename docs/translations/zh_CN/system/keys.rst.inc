图形模拟期间，可用下表快捷键切换模式。默认修饰键为 :kbd:`Ctrl+Alt`，
可在支持时通过 ``-display`` 的 ``mod=`` 子选项修改。例如
``-display sdl,grab-mod=lshift-lctrl-lalt`` 改为 :kbd:`Ctrl+Alt+Shift`，
``-display sdl,grab-mod=rctrl`` 改为右侧 :kbd:`Ctrl`。

.. list-table:: 多路复用器按键
  :widths: 10 90
  :header-rows: 1

  * - 按键序列
    - 操作

  * - :kbd:`Ctrl+Alt+f`
    - 切换全屏

  * - :kbd:`Ctrl+Alt++`
    - 放大画面

  * - :kbd:`Ctrl+Alt+-`
    - 缩小画面

  * - :kbd:`Ctrl+Alt+0`
    - 恢复未缩放的尺寸

  * - :kbd:`Ctrl+Alt+n`
    - 切换到虚拟控制台 n，标准映射为：

      - *1*：目标系统显示
      - *2*：监视器
      - *3*：串口
  * - :kbd:`Ctrl+Alt+g`
    - 切换鼠标和键盘捕获状态。

虚拟控制台中，可用 :kbd:`Ctrl+Up`、:kbd:`Ctrl+Down`、:kbd:`Ctrl+PageUp`
和 :kbd:`Ctrl+PageDown` 浏览历史输出。
