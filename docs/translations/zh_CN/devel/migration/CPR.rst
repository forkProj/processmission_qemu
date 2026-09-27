检查点与重启（CPR）
===================

CPR 是一组迁移模式的统称，将虚拟机迁移到同一宿主上的新 QEMU 实例，
用于更新运行虚拟机的宿主软件，例如 QEMU，甚至宿主内核。
目前提供 cpr-reboot、cpr-transfer 和 cpr-exec。

因为仍在同一宿主重启，能够访问相同本地设备，
某些普通迁移会被阻止的场景可以使用 CPR。
但退出旧 QEMU 到启动新实例之间，不得修改客户机块设备内容。

CPR 在保存内存前总会停止虚拟机，因此不依赖脏页跟踪。

cpr-reboot 模式
---------------

QEMU 停止虚拟机，将状态写入迁移 URI，通常是文件。
退出后，用户通过带 -incoming 的 QEMU 恢复。
由于新旧实例不同时运行，URI 不能采用实例之间直接传输数据流的类型。

客户机 RAM 若由共享内存承载，可以原地保留；也可以复制到文件。
原地保留更高效，因此优先推荐。

保存状态和内存后，可以更新宿主用户空间软件，再重启 QEMU 恢复虚拟机。
如果 RAM 由 DAX 等持久共享内存承载，还可先重启到新宿主内核。

此模式支持 VFIO，但必须先挂起客户机，例如向已安装的客户机代理发送 guest-suspend-ram。
客户机需要支持挂起到 RAM。挂起可能耗时数秒，应轮询确认进入 suspended 状态后再继续。

用法
^^^^^^^^

建议使用 memory-backend-file,share=on 等共享内存，并启用 x-ignore-shared，
使内存原地保存。否则，停止虚拟机后会将全部 RAM 复制到迁移 URI。

迁出端：
  * 将迁移 mode 设为 cpr-reboot。
  * 按需启用 x-ignore-shared。
  * 发送 migrate。建议使用 file URI；也可用 exec 等，
    但必须确保完整保存迁出数据，并完整提供给迁入端。
  * 进入 postmigrate 后退出 QEMU。

迁入端：
  * 用 -incoming defer 启动。
  * 将迁移 mode 设为 cpr-reboot。
  * 按需启用 x-ignore-shared。
  * 发送 migrate-incoming。
  * 如果迁出时虚拟机正在运行，QEMU 会自动恢复执行。

示例 1
^^^^^^^^
::

  # qemu-kvm -monitor stdio
  -object memory-backend-file,id=ram0,size=4G,mem-path=/dev/dax0.0,align=2M,share=on -m 4G
  ...

  (qemu) info status
  VM status: running
  (qemu) migrate_set_parameter mode cpr-reboot
  (qemu) migrate_set_capability x-ignore-shared on
  (qemu) migrate -d file:vm.state
  (qemu) info status
  VM status: paused (postmigrate)
  (qemu) quit

  ### optionally update kernel and reboot
  # systemctl kexec
  kexec_core: Starting new kernel
  ...

  # qemu-kvm ... -incoming defer
  (qemu) info status
  VM status: paused (inmigrate)
  (qemu) migrate_set_parameter mode cpr-reboot
  (qemu) migrate_set_capability x-ignore-shared on
  (qemu) migrate_incoming file:vm.state
  (qemu) info status
  VM status: running


示例 2：VFIO
^^^^^^^^^^^^
::

  # qemu-kvm -monitor stdio
  -object memory-backend-file,id=ram0,size=4G,mem-path=/dev/dax0.0,align=2M,share=on -m 4G
  -device vfio-pci, ...
  -chardev socket,id=qga0,path=qga.sock,server=on,wait=off
  -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0
  ...

  (qemu) info status
  VM status: running

  # echo '{"execute":"guest-suspend-ram"}' | ncat --send-only -U qga.sock

  (qemu) info status
  VM status: paused (suspended)
  (qemu) migrate_set_parameter mode cpr-reboot
  (qemu) migrate_set_capability x-ignore-shared on
  (qemu) migrate -d file:vm.state
  (qemu) info status
  VM status: paused (postmigrate)
  (qemu) quit

  ### optionally update kernel and reboot
  # systemctl kexec
  kexec_core: Starting new kernel
  ...

  # qemu-kvm ... -incoming defer
  (qemu) info status
  VM status: paused (inmigrate)
  (qemu) migrate_set_parameter mode cpr-reboot
  (qemu) migrate_set_capability x-ignore-shared on
  (qemu) migrate_incoming file:vm.state
  (qemu) info status
  VM status: paused (suspended)
  (qemu) system_wakeup
  (qemu) info status
  VM status: running


限制
^^^^^^^^

不能与 postcopy、background-snapshot 或 COLO 同时使用。

cpr-transfer 模式
-----------------

此模式通过原地保留 RAM，将客户机以最短暂停时间转交同宿主的新 QEMU。
新进程中的虚拟地址可能不同。VFIO 和 IOMMUFD 的设备及其固定内存页也会保留。

像普通在线迁移一样，启动新 QEMU，使用创建相同机器的参数，
并通过 -incoming 指定主迁移通道。
此外，再添加一个类型为 cpr 的 -incoming 通道。
它必须支持 SCM_RIGHTS 文件描述符传递，因此必须是 Unix 域 socket。

向旧 QEMU 发送 migrate，并在 channels 中加入第二个 cpr 通道，即可启动迁移。
旧实例停止虚拟机，将状态写入通道并进入 postmigrate，新实例恢复执行。

新 QEMU 在打开监视器之前读取 CPR 通道，
因此不能通过 migrate-incoming 的通道列表指定它，只能从命令行指定。

用法
^^^^^^^^

内存后端必须设置 share=on。

虚拟机必须用 ``-machine aux-ram-share=on`` 启动，
使未由 memory-backend 描述的隐式 RAM 块通过 mmap memfd 分配，例如 VGA 和 ROM。

迁出端：
  * 将 mode 设为 cpr-transfer。
  * 发送包含主通道和 cpr 通道的 migrate。

迁入端：
  * 用两个 -incoming 选项启动新 QEMU。
  * 如果迁出时虚拟机正在运行，会自动恢复。

限制
^^^^^^^^

不能与 postcopy、background-snapshot 或 COLO 同时使用。

不支持 memory-backend-epc。

主迁入通道不能是 file 类型。

主通道使用 inet socket 时，端口不能为 0，即不能动态选择端口。

使用 -incoming defer 时，必须先向旧 QEMU 发送 migrate，才能向新实例发送监视器命令。
因为新实例会在启动监视器前等待 CPR 通道，而旧实例在收到 migrate 前不会写入。
但新实例只有收到 migrate incoming 后，才会打开和读取主通道。

示例 1：指定迁入通道
^^^^^^^^^^^^^^^^^^^^

以下示例只重启同版本 QEMU，实际使用时应在迁入端运行新版本。
旧实例发出 migrate 前，新实例不会显示监视器提示符。
迁出端使用 QMP，因为 HMP 无法指定 CPR 通道。为简洁起见，省略部分 QMP 应答。

::

  Outgoing:                             Incoming:

  # qemu-kvm -qmp stdio
  -object memory-backend-file,id=ram0,size=4G,
  mem-path=/dev/shm/ram0,share=on -m 4G
  -machine memory-backend=ram0
  -machine aux-ram-share=on
  ...
                                        # qemu-kvm -monitor stdio
                                        -incoming tcp:0:44444
                                        -incoming '{"channel-type": "cpr",
                                          "addr": { "transport": "socket",
                                          "type": "unix", "path": "cpr.sock"}}'
                                        ...
  {"execute":"qmp_capabilities"}

  {"execute": "query-status"}
  {"return": {"status": "running",
              "running": true}}

  {"execute":"migrate-set-parameters",
   "arguments":{"mode":"cpr-transfer"}}

  {"execute": "migrate", "arguments": { "channels": [
    {"channel-type": "main",
     "addr": { "transport": "socket", "type": "inet",
               "host": "0", "port": "44444" }},
    {"channel-type": "cpr",
     "addr": { "transport": "socket", "type": "unix",
               "path": "cpr.sock" }}]}}

                                        QEMU 10.0.50 monitor
                                        (qemu) info status
                                        VM status: running

  {"execute": "query-status"}
  {"return": {"status": "postmigrate",
              "running": false}}


示例 2：延迟迁入
^^^^^^^^^^^^^^^^

使用 -incoming defer，在接受主迁移通道前热插入设备。
同样必须先向旧实例发送 migrate，才能向新实例发送监视器命令。

::

  Outgoing:                             Incoming:

  # qemu-kvm -monitor stdio
  -object memory-backend-file,id=ram0,size=4G,
  mem-path=/dev/shm/ram0,share=on -m 4G
  -machine memory-backend=ram0
  -machine aux-ram-share=on
  ...
                                        # qemu-kvm -monitor stdio
                                        -incoming defer
                                        -incoming '{"channel-type": "cpr",
                                          "addr": { "transport": "socket",
                                          "type": "unix", "path": "cpr.sock"}}'
                                        ...
  {"execute":"qmp_capabilities"}

  {"execute": "device_add",
   "arguments": {"driver": "pcie-root-port"}}

  {"execute":"migrate-set-parameters",
   "arguments":{"mode":"cpr-transfer"}}

  {"execute": "migrate", "arguments": { "channels": [
    {"channel-type": "main",
     "addr": { "transport": "socket", "type": "inet",
               "host": "0", "port": "44444" }},
    {"channel-type": "cpr",
     "addr": { "transport": "socket", "type": "unix",
               "path": "cpr.sock" }}]}}

                                        QEMU 10.0.50 monitor
                                        (qemu) info status
                                        VM status: paused (inmigrate)
                                        (qemu) device_add pcie-root-port
                                        (qemu) migrate_incoming tcp:0:44444
                                        (qemu) info status
                                        VM status: running

  {"execute": "query-status"}
  {"return": {"status": "postmigrate",
              "running": false}}


未来方向
^^^^^^^^

cpr-transfer 的基础是将打开的文件描述符从旧实例传给新实例。
未来也可传递 vhost 和字符设备描述符，在不中断的情况下保留设备及内核状态，
即使它们没有显式支持在线迁移。

cpr-exec 模式
-------------

QEMU 停止虚拟机，将状态写入 URI，再直接 exec 同宿主的新 QEMU，
替换原进程但保留 PID。RAM 原地保留，虚拟地址可能不同。
用户通过 -incoming，以及必要时的 migrate-incoming 完成迁移，详见下文。

设备描述符和内核状态跨 exec 保留，因此支持 VFIO/IOMMUFD，
即使设备本身不支持在线迁移。

新旧实例不同时运行，URI 不能采用实例间直接传输数据流的类型。

此模式无需 cpr 通道。
cpr-transfer 原本通过该通道传递的信息，会序列化到 memfd。
exec 新实例时将描述符编号保存到 QEMU_CPR_EXEC_STATE 环境变量，
新实例再 mmap 该 memfd。

用法
^^^^^^^^

新进程参数来自 @cpr-exec-command。
首个参数应是新 QEMU 路径，或会 exec 新 QEMU 的前缀命令，参数必须包含 -incoming。

内存后端必须设置 share=on，虚拟机必须用 -machine aux-ram-share=on 启动。

迁出端：
  * 将 mode 设为 cpr-exec。
  * 设置 cpr-exec-command。
  * 发送 migrate。建议使用 file URI，也可用 exec 等，
    但必须完整保存迁出数据并提供给迁入端。

迁入端：
  * 无需手动启动新 QEMU，migrate 会自动启动它。
  * 如果迁出时虚拟机正在运行，会自动恢复。

示例 1：指定迁入 URI
^^^^^^^^^^^^^^^^^^^^

以下示例仅重启同版本，实际使用时应在 cpr-exec-command 中设置新程序路径。

::

  # qemu-kvm -monitor stdio
  -object memory-backend-memfd,id=ram0,size=4G
  -machine memory-backend=ram0
  -machine aux-ram-share=on
  ...

  QEMU 10.2.50 monitor - type 'help' for more information
  (qemu) info status
  VM status: running
  (qemu) migrate_set_parameter mode cpr-exec
  (qemu) migrate_set_parameter cpr-exec-command qemu-kvm ... -incoming file:vm.state
  (qemu) migrate -d file:vm.state
  (qemu) QEMU 10.2.50 monitor - type 'help' for more information
  (qemu) info status
  VM status: running


示例 2：延迟迁入
^^^^^^^^^^^^^^^^
::

  # qemu-kvm -monitor stdio
  -object memory-backend-memfd,id=ram0,size=4G
  -machine memory-backend=ram0
  -machine aux-ram-share=on
  ...

  QEMU 10.2.50 monitor - type 'help' for more information
  (qemu) info status
  VM status: running
  (qemu) migrate_set_parameter mode cpr-exec
  (qemu) migrate_set_parameter cpr-exec-command qemu-kvm ... -incoming defer
  (qemu) migrate -d file:vm.state
  (qemu) QEMU 10.2.50 monitor - type 'help' for more information
  (qemu) info status
  status: paused (inmigrate)
  (qemu) migrate_incoming file:vm.state
  (qemu) info status
  VM status: running


限制
^^^^^^^^

不能与 postcopy、background-snapshot 或 COLO 同时使用。

需要 exec 系统调用权限，spawn 等沙箱选项会禁止它。

大容量客户机 RAM 使用小页承载时，暂停时间会增加。
