VirtIO 音频
============

本文介绍半虚拟化 Virtio 声卡的设置和使用。

Linux 内核支持
--------------------

客户机 Linux 内核必须启用 CONFIG_SND_VIRTIO。

说明
-----------

它通过主机配置的音频后端，在客户机内实现采集和播放。

设备属性
-----------------

可配置以下属性：

 * ``jacks``：物理插孔数量，尚未实现。
 * ``streams``：PCM 流数量。目前不支持配置流：第一条始终用于播放，可选的第二条用于采集，后续流按播放、采集交替。
 * ``chmaps``：声道映射数量，尚未实现。

所有流均为立体声，默认声道位置是前左和前右。

示例
--------

用 -audio 和 model=virtio 同时添加声卡与后端：

 * pulseaudio: ``-audio driver=pa,model=virtio``
   或 ``-audio driver=pa,model=virtio,server=/run/user/1000/pulse/native``
 * sdl: ``-audio driver=sdl,model=virtio``
 * coreaudio: ``-audio driver=coreaudio,model=virtio``

等等。

若单独添加虚拟声卡，需指定 PCI 设备，并选择 -audio driver=help 列出的、
能在主机工作的后端，例如：

::

  -device virtio-sound-pci,audiodev=my_audiodev \
  -audiodev alsa,id=my_audiodev
