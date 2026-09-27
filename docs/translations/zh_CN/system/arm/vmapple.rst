.. SPDX-License-Identifier: GPL-2.0-or-later

VMApple 机器模拟
========================================================================================

VMApple 是 macOS 内置虚拟机监控器 "Virtualization.framework" 向 Apple Silicon macOS 客户机提供的设备模型。QEMU 中的 "vmapple" 机器模型实现了相同的设备模型，但没有使用 Virtualization.Framework 的任何代码。

前提条件
-------------

要运行 vmapple 机器模型，需要：

 * 在 Apple Silicon 上运行
 * 运行 macOS 12.0 或更高版本
 * 已安装好的 Virtualization.Framework macOS 12 虚拟机副本。注意，客户机目前不支持高于 12.x 的版本。下文假设使用 `macosvm <https://github.com/s-u/macosvm>`__ 命令行工具完成安装。

首先，需要从已安装的虚拟机提取 UUID。可以对 macosvm.json 文件运行 contrib/vmapple/uuid.sh 中的 shell 脚本：

.. code-block:: bash
  :caption: uuid.sh script to extract the UUID from a macosvm.json file

  $ contrib/vmapple/uuid.sh "path/to/macosvm.json"


还需要裁剪 aux 分区，其中包含可以直接丢弃的元数据：

.. code-block:: bash
  :caption: Command to trim the aux file

  $ dd if="aux.img" of="aux.img.trimmed" bs=$(( 0x4000 )) skip=1


运行方式
----------

随后，可以使用 Virtualization.Framework 预启动环境和已安装好的目标磁盘镜像启动 QEMU。建议将虚拟机的 SSH 和 VNC 端口转发到主机，以便更方便地交互访问目标系统：

.. code-block:: bash
  :caption: Example execution command line

  $ UUID="$(contrib/vmapple/uuid.sh 'macosvm.json')"
  $ AVPBOOTER="/System/Library/Frameworks/Virtualization.framework/Resources/AVPBooter.vmapple2.bin"
  $ AUX="aux.img.trimmed"
  $ DISK="disk.img"
  $ qemu-system-aarch64 \
       -serial mon:stdio \
       -m 4G \
       -accel hvf \
       -M vmapple,uuid="$UUID" \
       -bios "$AVPBOOTER" \
       -drive file="$AUX",if=pflash,format=raw \
       -drive file="$DISK",if=pflash,format=raw \
       -drive file="$AUX",if=none,id=aux,format=raw \
       -drive file="$DISK",if=none,id=root,format=raw \
       -device vmapple-virtio-blk-pci,variant=aux,drive=aux \
       -device vmapple-virtio-blk-pci,variant=root,drive=root \
       -netdev user,id=net0,ipv6=off,hostfwd=tcp::2222-:22,hostfwd=tcp::5901-:5900 \
       -device virtio-net-pci,netdev=net0
