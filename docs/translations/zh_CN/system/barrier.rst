QEMU Barrier 客户端
===================

通常通过 QEMU 视频界面模拟捕获鼠标和键盘。

使用 PCI 直通显卡时，若不为主机另接一套鼠标键盘，或在客户机操作系统中
安装 KVM 软件，就无法向虚拟机提供键鼠输入。

QEMU Barrier 客户端直接在 QEMU 中实现 Barrier 协议，解决这一问题。

`Barrier <https://github.com/debauchee/barrier>`__ 是从 Symless synergy 1.9
代码派生的键盘、视频、鼠标共享软件。

向 QEMU 添加 input-barrier 对象即可启用，语法为::

    input-barrier,id=<object-id>,name=<guest display name>
    [,server=<barrier server address>][,port=<barrier server port>]
    [,x-origin=<x-origin>][,y-origin=<y-origin>]
    [,width=<width>][,height=<height>]

例如，在命令行加入::

    -object input-barrier,id=barrier0,name=VM-1

VM-1 是提供键鼠事件的主机 Barrier 服务器中配置的显示名称。

默认 ``<barrier server address>`` 为 ``localhost``，``<port>`` 为 ``24800``，
``<x-origin>``、``<y-origin>`` 为 ``0``，``<width>``、``<height>`` 分别为
``1920``、``1080``。

服务器停止后，需要删除并重新添加对象来手动重连，例如使用 HMP 监视器::

    (qemu) object_del barrier0
    (qemu) object_add input-barrier,id=barrier0,name=VM-1
