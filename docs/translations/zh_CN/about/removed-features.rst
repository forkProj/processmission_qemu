已移除功能
==========

本节记录近期移除、此前已弃用的功能，供升级后遇到问题的用户查阅。

系统模拟器命令行参数
--------------------

``-hdachs`` （2.12 移除）
'''''''''''''''''''''''''

原 ``-hdachs c,h,s,t`` 指定的磁盘几何参数，应改用
``-device ide-hd,drive=dr,cyls=c,heads=h,secs=s,bios-chs-trans=t``，
并配合 ``-drive if=none,id=dr,...``。

``-net channel`` （2.12 移除）
''''''''''''''''''''''''''''''

改用 ``-net user,guestfwd=...``。

``-net dump`` （2.12 移除）
'''''''''''''''''''''''''''

``-net dump[,vlan=n][,file=filename][,len=maxlen]`` 由
``-object filter-dump,id=id,netdev=dev[,file=filename][,maxlen=maxlen]`` 取代。
新语法使用 netdev ID，而非旧 vlan 集线器。

``-no-kvm-pit`` （2.12 移除）
'''''''''''''''''''''''''''''

该选项早已被忽略，因为内核 PIT 无法再独立于 irqchip 禁用。
使用 ``-M kernel_irqchip=split`` 可获得类似效果，同时也禁用 KVM IOAPIC。

``-tdf`` （2.12 移除）
''''''''''''''''''''''

没有替代选项。它在 qemu-kvm 中控制的行为现已成为 KVM PIT 的默认行为，
因此该选项早已被忽略。仍可用 ``-global kvm-pit.lost_tick_policy=delay`` 显式请求。

``-drive secs=s``、``-drive heads=h`` 和 ``-drive cyls=c`` （3.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

磁盘几何参数改用 ``-device ...,drive=dr,cyls=c,heads=h,secs=s``，
配合 ``-drive if=none,id=dr,...``。

``-drive serial=``、``-drive trans=`` 和 ``-drive addr=`` （3.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``-device ...,drive=dr,serial=r,bios-chs-trans=t,addr=a``，
配合 ``-drive if=none,id=dr,...``。

``-net ...,vlan=x`` （3.0 移除）
''''''''''''''''''''''''''''''''

这里的 vlan 指集线器 ID，并非 IEEE 802.1Q，因此容易混淆，现已移除。
将一个网卡前端连接到后端时，可对板载网卡使用 ``-nic ...``，
或用 ``-netdev ...,id=n`` 配合 ``-device ...,netdev=n``，完整控制可插拔网卡。
若要像旧 vlan 一样通过集线器连接多个网卡或后端，
使用 ``-nic hubport,hubid=x,...``，
或 ``-netdev hubport,id=n,hubid=x,...`` 配合 ``-device ...,netdev=n``。

``-no-kvm-irqchip`` （3.0 移除）
''''''''''''''''''''''''''''''''

改用 ``-machine kernel_irqchip=off``。

``-no-kvm-pit-reinjection`` （3.0 移除）
''''''''''''''''''''''''''''''''''''''''

改用 ``-global kvm-pit.lost_tick_policy=discard``。

``-balloon`` （3.1 移除）
'''''''''''''''''''''''''

``-balloon virtio`` 由 ``-device virtio-balloon`` 取代。
``-balloon none`` 原本没有作用，无替代选项。

``-bootp`` （3.1 移除）
'''''''''''''''''''''''

可插拔网卡使用 ``-netdev user,id=x,bootp=/some/file``，
配合 ``-device ...,netdev=x``；板载网卡使用 ``-nic user,bootp=/some/file``，
替代 ``-bootp /some/file``。新语法可为各网卡分别配置。

``-redir`` （3.1 移除）
'''''''''''''''''''''''

``-redir [tcp|udp]:hostport:[guestaddr]:guestport`` 改用
``-netdev user,id=x,hostfwd=[tcp|udp]:[hostaddr]:hostport-[guestaddr]:guestport``
并配合 ``-device ...,netdev=x``，适用于可插拔网卡；板载网卡则使用
``-nic user,hostfwd=[tcp|udp]:[hostaddr]:hostport-[guestaddr]:guestport``。
新语法可为各网卡分别配置。

``-smb`` （3.1 移除）
'''''''''''''''''''''

``-smb /some/dir`` 改为可插拔网卡的 ``-netdev user,id=x,smb=/some/dir``
配合 ``-device ...,netdev=x``，或板载网卡的 ``-nic user,smb=/some/dir``。
新语法可为各网卡分别配置。

``-tftp`` （3.1 移除）
''''''''''''''''''''''

``-tftp /some/dir`` 改为可插拔网卡的 ``-netdev user,id=x,tftp=/some/dir``
配合 ``-device ...,netdev=x``，或板载网卡的 ``-nic user,tftp=/some/dir``。
新语法可为各网卡分别配置。

``-localtime`` （3.1 移除）
'''''''''''''''''''''''''''

改用 ``-rtc base=localtime``。

``-nodefconfig`` （3.1 移除）
'''''''''''''''''''''''''''''

改用 ``-no-user-config``。

``-rtc-td-hack`` （3.1 移除）
'''''''''''''''''''''''''''''

改用 ``-rtc driftfix=slew``。

``-startdate`` （3.1 移除）
'''''''''''''''''''''''''''

改用 ``-rtc base=date``。

``-vnc ...,tls=...``、``-vnc ...,x509=...`` 和 ``-vnc ...,x509verify=...`` （3.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 tls-creds，指向通过 -object 创建的 tls-creds-x509 对象。

``-mem-path`` 失败后回退到 RAM（5.0 移除）
''''''''''''''''''''''''''''''''''''''''''

过去，从 mem-path 文件分配客户机 RAM 失败时，QEMU 会回退到普通 RAM。
这忽略了用户指定的后备文件，可能产生不可预测行为。
现在，用户必须确保 -mem-path 的存储能够提供 -m 配置的内存，
分配失败时 QEMU 会拒绝启动。

``-net ...,name=...`` （5.1 移除）
''''''''''''''''''''''''''''''''''

name 是 id 的别名，现在应使用 id。

RISC-V 默认不启动固件的行为（5.1 移除）
'''''''''''''''''''''''''''''''''''''''

QEMU 5.1 将 RISC-V virt 和 sifive_u 机器的默认行为，
从 ``-bios none`` 改为 ``-bios default``。

``-numa node,mem=...`` （5.1 移除）
'''''''''''''''''''''''''''''''''''

mem 用于将部分客户机 RAM 分配给 NUMA 节点，但无法在宿主侧管理这块内存，
例如绑定到宿主节点或设置绑定策略，因此只是性能不佳的虚拟 NUMA 配置。
从 2014 年起，memdev 提供相同功能，并允许在宿主侧实际管理节点 RAM。
用 memory-backend-ram 配合 memdev，可保留类似旧 mem 的虚拟 NUMA 效果；
正确配置 memory-backend-file 则能真正利用 NUMA。
5.1 起的新机器版本不再接受 mem，但旧类型仍可使用。
可通过 QAPI 的 MachineInfo::numa-mem-supported 检查是否支持旧选项。

未指定内存的 ``-numa`` 节点（5.2 移除）
'''''''''''''''''''''''''''''''''''''''

默认将 RAM 分配到 NUMA 节点，与 mem 存在相同问题，
只不过分配由 QEMU 按通用或开发板专用规则隐式完成。
应使用 memdev 配合 memory-backend-ram，或在机器支持时使用 mem，显式定义映射。
现有虚拟机若需保持原分布，应通过 ``-numa node,memdev`` 显式配置。
用 HMP ``info numa`` 查看当前分布；存在独立内存设备（pc 或 nv-dimm）时，
还应通过 ``info memory-device`` 查出设备内存，并从 info numa 结果中减去。

``-smp`` 的无效拓扑（5.2 移除）
'''''''''''''''''''''''''''''''

CPU 拓扑应覆盖整台机器，包括所有可能的 CPU。
过去允许 ``n <= sockets * cores * threads < maxcpus``，
可能导致客户机错误枚举拓扑。
现在必须保证 ``sockets * cores * threads = maxcpus``，包含全部可能的 CPU。

``-machine enforce-config-section=on|off`` （5.2 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``-global migration.send-configuration={on|off}``。

``-no-kvm`` （5.2 移除）
''''''''''''''''''''''''

此参数是 ``-machine accel=tcg`` 的别名。

``-realtime`` （6.0 移除）
''''''''''''''''''''''''''

``-realtime mlock=on|off`` 由 ``-overcommit mem-lock=on|off`` 取代。

``-show-cursor`` （6.0 移除）
'''''''''''''''''''''''''''''

改用 ``-display sdl,show-cursor=on``、``-display gtk,show-cursor=on``
或 ``-display default,show-cursor=on``。

``-tb-size`` （6.0 移除）
'''''''''''''''''''''''''

QEMU 5.0 引入了设置翻译块缓存大小的新语法 ``-accel tcg,tb-size=``。

``-usbdevice audio`` （6.0 移除）
'''''''''''''''''''''''''''''''''

旧选项无法指定音频后端。现在使用 ``-device usb-audio``，
必要时同时指定 USB 主控制器或 ``-usb``。

``-vnc acl`` （6.0 移除）
'''''''''''''''''''''''''

acl 由 tls-authz 和 sasl-authz 取代。

``-mon ...,control=readline,pretty=on|off`` （6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''

pretty 对 HMP 监视器没有作用，因此拒绝使用。

``-drive file=json:{...{'driver':'file'}}`` （6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''

file 驱动不再用于字符设备或宿主设备，只接受普通文件 S_IFREG。
这些设备应按实际类型使用 host_cdrom 或 host_device。

软盘控制器的 drive 属性（6.0 移除）
'''''''''''''''''''''''''''''''''''

改用 ``-device floppy,...``。配置板载控制器时：
::

    -global isa-fdc.driveA=...
    -global sysbus-fdc.driveA=...
    -global SUNW,fdtwo.drive=...


改为：
::

    -device floppy,unit=0,drive=...


以及：
::

    -global isa-fdc.driveB=...
    -global sysbus-fdc.driveB=...


改为：
::

    -device floppy,unit=1,drive=...


插入软盘控制器时：
::

    -device isa-fdc,...,driveA=...


改为：
::

    -device isa-fdc,...
    -device floppy,unit=0,drive=...


以及：
::

    -device isa-fdc,...,driveB=...


改为：
::

    -device isa-fdc,...
    -device floppy,unit=1,drive=...


``-drive`` 使用无效接口类型（6.0 移除）
'''''''''''''''''''''''''''''''''''''''

除 if=none 外的接口类型用于板载设备。
未被开发板接收的驱动器不能再通过 -device 使用，应改用 ``if=none``。

``-usbdevice ccid`` （6.0 移除）
''''''''''''''''''''''''''''''''

旧选项未文档化，也没有实际用户，改用 ``-device usb-ccid``。

``-no-quit`` （7.0 移除）
'''''''''''''''''''''''''

它是 ``-display ...,window-close=off`` 的别名，应使用后者。

``--enable-fips`` （7.1 移除）
''''''''''''''''''''''''''''''

宿主运行于 FIPS 模式时，该选项用于限制特定密码算法。

需要 FIPS 合规时，应启用 libgcrypt 或 gnutls 作为密码后端构建 QEMU。

nettle 和内置密码后端不支持启用 FIPS 的宿主。

``-writeconfig`` （7.1 移除）
'''''''''''''''''''''''''''''

它不能完整序列化 QEMU 命令行，因此被视为失败的尝试，移除且无替代功能。

secret 和 TLS 凭据对象的 ``loaded`` 属性（9.2 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''

命令行或 QMP object-add 中的 loaded=on，若为最后一个选项则无作用，
否则会使部分选项像未提供一样被忽略。因此该无用属性被移除。

``rng-*`` 对象的 ``opened`` 属性（7.1 移除）
''''''''''''''''''''''''''''''''''''''''''''

opened=on 若为最后一个选项则无作用，否则会导致错误，应直接删除该属性。

``-display sdl,window_close=...`` （7.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''

改用 ``-display sdl,window-close=...``，即 window 与 close 之间使用连字符而非下划线。

``-alt-grab`` 和 ``-display sdl,alt_grab=on`` （7.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``-display sdl,grab-mod=lshift-lctrl-lalt``。

``-ctrl-grab`` 和 ``-display sdl,ctrl_grab=on`` （7.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``-display sdl,grab-mod=rctrl``。

``-sdl`` （7.1 移除）
'''''''''''''''''''''

改用 ``-display sdl``。

``-curses`` （7.1 移除）
''''''''''''''''''''''''

改用 ``-display curses``。

通过 ``-soundhw`` 创建声卡（7.1 移除）
''''''''''''''''''''''''''''''''''''''

应使用 -device 或 -audio 创建声卡。
例外是 pcspk，可通过 ``-machine pcspk-audiodev=<name>`` 启用。

``-watchdog`` （7.2 移除）
''''''''''''''''''''''''''

改用 ``-device``。

带倍率后缀的十六进制大小（8.0 移除）
''''''''''''''''''''''''''''''''''''

只有十进制值可以使用 k、M 等大小后缀，十六进制值不允许。
例如 0x20M 应写成 32M 或 0x2000000。

``-chardev`` 后端别名 ``tty`` 和 ``parport`` （8.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''

二者分别是 serial 和 parallel 的别名，应改用实际后端名称。

sifive_u OTP 设备的 ``-drive if=none`` （8.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``-drive if=pflash`` 配置该 RISC-V 机器的 OTP 设备。

``-spice password=string`` （8.0 移除）
'''''''''''''''''''''''''''''''''''''''

密码会出现在进程列表中，不安全。
新 password-secret 选项允许通过 secret 对象安全地提供密码。

``QEMU_AUDIO_`` 环境变量和 ``-audio-help`` （8.2 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''

现在只能通过 -audiodev 和 -audio 命令行选项配置音频后端。

用 ``-audiodev`` 定义默认音频后端（8.2 移除）
'''''''''''''''''''''''''''''''''''''''''''''

未指定 audiodev 属性时，旧版会回退到第一个 -audiodev。
从 8.2 起，这些后端只能被通过 audiodev= 显式引用它们的声卡、板载音频或 VNC 使用。

要设置默认后端，使用不带 model 的 -audio。
旧版本要求 model，但 8.2 起可以省略，此时不创建声卡。

使用 -nodefaults 时，必须在命令行配置默认音频后端。

``-no-hpet`` （9.0 移除）
'''''''''''''''''''''''''

HPET 已改为机器属性，使用 ``-machine hpet=off``。

``-no-acpi`` （9.0 移除）
'''''''''''''''''''''''''

ACPI 已改为机器属性，使用 ``-machine acpi=off``。

``-async-teardown`` （9.0 移除）
''''''''''''''''''''''''''''''''

改用 ``-run-with async-teardown=on``。

``-chroot`` （9.0 移除）
''''''''''''''''''''''''

改用 ``-run-with chroot=dir``。

``-singlestep`` （9.0 移除）
''''''''''''''''''''''''''''

该选项改为加速器属性，并采用更准确的名称。
使用 ``-accel tcg,one-insn-per-tb=on``。

``-smp`` 中 parameter=0 的配置（9.0 移除）
''''''''''''''''''''''''''''''''''''''''''

显式给出的 CPU 拓扑参数必须大于零。
用户应提供合理正值，或省略参数让 QEMU 计算。
过去隐式允许零值，既无意义，也可能导致 -smp 解析异常。
从 9.0 起不再支持 ``-smp 8,sockets=0`` 等配置，所有指定拓扑成员必须大于零。

``-global migration.decompress-error-check`` （9.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''

随 compression 迁移能力一起移除。

``-device virtio-blk,scsi=on|off`` （9.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''

virtio-blk 的 SCSI 直通属于旧 VIRTIO 功能，VIRTIO 1.0 及以后不再支持，
因为 virtio-scsi 已提供完整 SCSI 支持。需要直通时应使用 virtio-scsi。

``-fsdev proxy`` 和 ``-virtfs proxy`` （9.2 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''

9p proxy 后端最初把底层文件操作从 QEMU 的 9p 服务器交给独立的
virtfs-proxy-helper，意图提高安全性。但它比 local 慢很多，多年无人开发，
实际安全性也更差，尤其辅助守护进程必须以 root 运行。

改用 local，必要时通过 mapped 安全模型映射权限，
或切换到 virtiofs。virtiofsd 使用 vhost，避免了 9p proxy 的高延迟。

``-portrait`` 和 ``-rotate`` （9.2 移除）
'''''''''''''''''''''''''''''''''''''''''

文档规定这两个选项只用于 PXA LCD；使用该设备的机器已在 9.2 全部移除，
因此选项也随之移除。

它们用于模拟用户旋转移动设备，产生三种效果：

* 显示旋转 90、180 或 270 度。
* 鼠标或触控板输入反向旋转。
* 机器模型向客户机报告方向。

其中输入旋转没有限制在完整支持旋转的开发板上，
理论上可在其他机器产生只旋转输入、不旋转显示的怪异效果。
但这不是设计或文档承诺的行为，因此没有单独保留。

``-runas`` （10.0 移除）
''''''''''''''''''''''''

改用 ``-run-with user=..``。

通过 param_struct 启动 Arm 内核的 ``-old-param`` （10.2 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

此 Arm 专用选项在直接启动客户机内核时，用旧 param_struct ABI
传递命令行等信息，而非较新的 ATAGS 或 DTB。
只有 akita、terrier 等旧开发板上的古老内核需要它，内核自 2001 年起已弃用。
QEMU 现有机器均不再需要，因此移除。

32 位宿主操作系统（11.0 移除）
''''''''''''''''''''''''''''''

维护 32 位宿主支持负担较重，因此 QEMU 已停止支持所有 32 位宿主系统。

用户态模拟器命令行参数
----------------------

``-singlestep`` （9.0 移除）
''''''''''''''''''''''''''''

linux-user 和 bsd-user 均改用名称更准确的 ``-one-insn-per-tb``。

``-p`` （10.2 移除）
''''''''''''''''''''

此选项声称控制宿主页大小，但宿主页大小无法修改。
从 9.0 起它仅打印警告，现在完全移除。

QEMU 机器协议（QMP）命令
------------------------

``block-dirty-bitmap-add`` 的 autoload 参数（4.2 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''

autoload 从 2.12.0 起已被忽略，qcow2 镜像中的位图都会自动加载。

``cpu-add`` （5.2 移除）
''''''''''''''''''''''''

热插入 vCPU 改用 ``device_add``，详见 query-hotpluggable-cpus 文档。

``change`` （6.0 移除）
'''''''''''''''''''''''

改用 ``blockdev-change-medium``、``change-vnc-password`` 或 ``display-update``。

``query-events`` （6.0 移除）
'''''''''''''''''''''''''''''

由功能更强、信息更准确的 ``query-qmp-schema`` 取代。

``migrate_set_cache_size`` 和 ``query-migrate-cache-size`` （6.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``migrate_set_parameter`` 和 ``info migrate_parameters``。

``migrate_set_downtime`` 和 ``migrate_set_speed`` （6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``migrate_set_parameter``。

``query-cpus`` （6.0 移除）
'''''''''''''''''''''''''''

改用 ``query-cpus-fast``。

``query-cpus-fast`` 的 ``arch`` 输出成员（6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 target 输出成员。

字符设备客户端 socket 的 ``wait`` 选项（6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''

wait 只适用于服务器模式，创建客户端 socket 的字符设备不应指定该字段。

``query-named-block-nodes`` 的 ``encryption_key_missing`` 结果（6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

已移除，无替代项。

``query-block`` 的 ``inserted.encryption_key_missing`` 结果（6.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

已移除，无替代项。

``query-named-block-nodes`` 和 ``query-block`` 的 dirty-bitmaps[i].status 结果（6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

返回的 BlockDirtyInfo 中 status 被移除，由 recording 和 busy 两个布尔字段取代。

``query-block`` 的 ``dirty-bitmaps`` 结果字段（6.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''

BlockInfo 的 dirty-bitmaps 已移除，改用 BlockDeviceInfo 的同名字段。
后者是 query-block 返回值中 inserted 的类型，也是 query-named-block-nodes 数组元素的类型。

``object-add`` 的 ``props`` 选项（6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''

改为通过顶层参数指定对象属性。

``query-sgx`` 的 ``section-size`` 返回成员（8.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 sections。

``query-sgx-capabilities`` 的 ``section-size`` 返回成员（8.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 sections。

``query-migrate`` 的 ``skipped`` 返回成员（9.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''

MigrationRAMStats 的 skipped 已十多年未使用，移除且无替代项。

``migrate`` 的 ``inc`` 选项（9.1 移除）
'''''''''''''''''''''''''''''''''''''''

改用 blockdev-mirror 与 NBD。详细说明见 docs/interop/live-block-operations.rst
中使用 blockdev-mirror + NBD 进行在线存储迁移的 QMP 调用一节。

``migrate`` 的 ``blk`` 选项（9.1 移除）
'''''''''''''''''''''''''''''''''''''''

改用 blockdev-mirror 与 NBD。详细说明见 docs/interop/live-block-operations.rst
中使用 blockdev-mirror + NBD 进行在线存储迁移的 QMP 调用一节。

``migrate`` 使用基于文件的 ``fd:`` URI（11.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''

为减少歧义，不再通过 fd: URI 提供普通文件描述符。
应显式使用 file: URI，并以 fdset 传递描述符，详见 add-fd 文档。

``migrate-set-capabilities`` 的 ``block`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

块迁移已移除。替代方案见 docs/interop/live-block-operations.rst
中使用 blockdev-mirror + NBD 进行在线存储迁移的 QMP 调用一节。

``migrate-set-capabilities`` 的 ``zero-blocks`` 选项（11.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

9.1 移除块迁移时意外遗留了此能力，现在移除，无替代项。

``migrate-set-parameter`` 的 ``compress-level`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-zlib-level`` 或 ``multifd-zstd-level``。

``migrate-set-parameter`` 的 ``compress-threads`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-channels``。

``migrate-set-parameter`` 的 ``compress-wait-thread`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

已移除，无替代项。

``migrate-set-parameter`` 的 ``decompress-threads`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-channels``。

``migrate-set-capability`` 的 ``compress`` 选项（9.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-compression``。

类型错误的 ``device_add`` 参数（9.2 移除）
''''''''''''''''''''''''''''''''''''''''''

由于内部实现缺陷，过去会错误接受一些无效参数：对象和列表参数被静默忽略，
其他参数不检查类型而做隐式转换，例如将字符串赋给整数属性，或反向转换。

``query-migrationthreads`` （11.0 移除）
''''''''''''''''''''''''''''''''''''''''

只报告有限线程，例如仅报告 multifd 源端，不报告目标端或非 multifd 源端线程，
因此移除且无替代命令。调试时请使用 ``-name $VM,debug-threads=on``。

``migrate`` 的 ``detach`` 参数（11.0 移除）
'''''''''''''''''''''''''''''''''''''''''''

此参数一直被忽略。

QEMU 机器协议（QMP）事件
------------------------

``MEM_UNPLUG_ERROR`` （9.1 移除）
'''''''''''''''''''''''''''''''''

由更通用的 ``DEVICE_UNPLUG_GUEST_ERROR`` 取代。

``vcpu`` 跟踪事件（9.1 移除）
'''''''''''''''''''''''''''''

使用感知 vCPU 的跟踪点对 QEMU 辅助函数插桩的功能，已在 7.0 移除。

人机监视器协议（HMP）命令
-------------------------

``usb_add`` 和 ``usb_remove`` （2.12 移除）
'''''''''''''''''''''''''''''''''''''''''''

改用 ``device_add`` 和 ``device_del``；``device_add help`` 可列出设备。

``host_net_add`` 和 ``host_net_remove`` （2.12 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``netdev_add`` 和 ``netdev_del``。

``hostfwd_add`` / ``hostfwd_remove`` 的 ``hub_id`` 参数（5.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

二者的 ``[hub_id name]`` 参数组合已由 netdev_id 取代。

``cpu-add`` （5.2 移除）
''''''''''''''''''''''''

热插入 vCPU 改用 ``device_add``，详见 query-hotpluggable-cpus 文档。

``change vnc TARGET`` （6.0 移除）
''''''''''''''''''''''''''''''''''

无替代项，``change vnc password`` 和 ``change DEVICE MEDIUM`` 不受影响。

``acl_show``、``acl_reset``、``acl_policy``、``acl_add``、``acl_remove`` （6.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

这些命令移除后无直接替代。VNC 授权应通过可插拔 QAuthZ 对象实现。

``migrate-set-cache-size`` 和 ``info migrate-cache-size`` （6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``migrate-set-parameters`` 和 ``info migrate-parameters``。

``migrate_set_downtime`` 和 ``migrate_set_speed`` （6.0 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``migrate-set-parameters``。

``info cpustats`` （6.1 移除）
''''''''''''''''''''''''''''''

此命令原本已无输出，移除且无替代项。

``singlestep`` （9.0 移除）
'''''''''''''''''''''''''''

改用 ``one-insn-per-tb``，行为相同，名称更准确。

``migrate`` 的 ``-i`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''

改用 blockdev-mirror 与 NBD。详见 docs/interop/live-block-operations.rst
中相应的在线存储迁移 QMP 调用说明。

``migrate`` 的 ``-b`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''

改用 blockdev-mirror 与 NBD。详见 docs/interop/live-block-operations.rst
中相应的在线存储迁移 QMP 调用说明。

``migrate_set_capability`` 的 ``block`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''

块迁移已移除，替代方案见 docs/interop/live-block-operations.rst
中使用 blockdev-mirror + NBD 的在线存储迁移说明。

``migrate_set_parameter`` 的 ``compress-level`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-zlib-level`` 或 ``multifd-zstd-level``。

``migrate_set_parameter`` 的 ``compress-threads`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-channels``。

``migrate_set_parameter`` 的 ``compress-wait-thread`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

已移除，无替代项。

``migrate_set_parameter`` 的 ``decompress-threads`` 选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-channels``。

``migrate_set_capability`` 的 ``compress`` 选项（9.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

改用 ``multifd-compression``。

宿主架构
--------

32 位 Windows 宿主上的系统模拟（9.0 移除）
''''''''''''''''''''''''''''''''''''''''''

Windows 11 不支持 32 位安装，Windows 10 也仅支持从旧系统升级而非新装 32 位系统。
MSYS2 已停止支持 32 位 Windows，QEMU 也在整体停止支持 32 位 x86 宿主。
因此不再支持该平台。过去十多年的 x86 硬件均支持 64 位扩展，应使用 64 位系统。

32 位宿主上的 64 位客户机（10.0 移除）
''''''''''''''''''''''''''''''''''''''

32 位宿主通常无法满足 64 位客户机的地址空间或原子性要求。
10.0 之前，系统模式尝试通过将全部 vCPU 放在同一线程中绕过原子性问题，
而用户模式的原子性实际不正确。从 10.0 起禁止这种配置。

32 位 MIPS（10.2 移除）
'''''''''''''''''''''''

Debian 12 Bookworm 移除了 32 位 MIPS，难以继续维护交叉编译 CI。

64 位 MIPS（11.1 移除）
'''''''''''''''''''''''

Debian 13 Trixie 移除了 MIPS，难以继续维护交叉编译 CI。

32 位 PPC（10.2 移除）
''''''''''''''''''''''

QEMU 不再支持 32 位宿主构建。

客户机指令集
------------

RISC-V 特权规范 1.09.1（5.1 移除）
''''''''''''''''''''''''''''''''''

1.09.1 已移除，应使用支持的较新 1.10.0 或已批准的 1.11.0。

系统模拟器 CPU
--------------

32 位 Arm 宿主的 KVM 客户机支持（5.2 移除）
'''''''''''''''''''''''''''''''''''''''''''

Linux 5.7 起不再允许 32 位 Arm 宿主运行 KVM 客户机，QEMU 因此也移除了支持。
64 位 Arm 宿主运行 32 位客户机仍受支持。

名称包含 ISA 版本的 RISC-V CPU（5.1 移除）
''''''''''''''''''''''''''''''''''''''''''

移除了 ``rv32gcsu-v1.9.1``、``rv32gcsu-v1.10.0``、``rv64gcsu-v1.9.1``
和 ``rv64gcsu-v1.10.0``。
改用 rv32 或 rv64，并通过 priv_spec 指定版本。

无 MMU 的 RISC-V CPU（5.1 移除）
''''''''''''''''''''''''''''''''

移除了 ``rv32imacu-nommu`` 和 ``rv64imacu-nommu``。
改用 rv32 或 rv64，并通过 mmu 选项设置 MMU 状态。

服务器类 POWER CPU 的 ``compat`` 属性（6.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''

改用 pseries 机器的 max-cpu-compat。

``moxie`` CPU（6.1 移除）
'''''''''''''''''''''''''

没有用户，也没有测试镜像验证模拟功能，因此移除，无替代项。

``lm32`` CPU（6.1 移除）
''''''''''''''''''''''''

唯一公开用户 milkymist 项目已停止多年，该架构也从未进入上游 Linux。
移除且无替代项。

``unicore32`` CPU（6.1 移除）
'''''''''''''''''''''''''''''

上游 Linux 已移除支持，也没有可用的上游工具链，因此移除，无替代项。

x86 ``Icelake-Client`` CPU（7.1 移除）
''''''''''''''''''''''''''''''''''''''

该模型是错误定义的、并不存在的 Icelake Client CPU，应使用 Icelake-Server。

Nios II CPU（9.1 移除）
'''''''''''''''''''''''

QEMU 中该架构无人维护，Intel 也终止了 Nios II IP，见 `Intel discontinuance notification`_。

CRIS CPU 架构（9.2 移除）
'''''''''''''''''''''''''

Linux 4.17 移除了 CRIS，各发行版也不再打包其编译器，难以运行 check-tcg。

RISC-V 的 any CPU，即 ``-cpu any`` （9.2 移除）
'''''''''''''''''''''''''''''''''''''''''''''''

any 从 2018 年初始 RISC-V 移植起存在，但用途始终不清晰。
用户无法从名称判断行为，实际也没有提供默认 rv32/rv64 之外的功能。

Power8E 和 Power8NVL CPU（11.1 移除）
'''''''''''''''''''''''''''''''''''''

这些 Power8 变体已无明显用途，陈旧且无人维护，
因此连同相应的 Power8NVL 和 Power8E PnvChip 一起移除。

系统加速器
----------

KVM 的用户空间本地 APIC（x86，8.0 移除）
''''''''''''''''''''''''''''''''''''''''

CPU 模型含本地 APIC 时，KVM 不能使用 ``-M kernel-irqchip=off``。
split 仍受支持；没有本地 APIC 的 CPU 也仍可使用 off。

MIPS Trap-and-Emulate KVM 支持（8.0 移除）
''''''''''''''''''''''''''''''''''''''''''

Linux 在 2021 年移除了该 KVM 宿主与客户机支持，QEMU 也不再支持。

HAXM（``-accel hax``，8.2 移除）
''''''''''''''''''''''''''''''''

HAXM 已停止维护，见 https://github.com/intel/haxm#status。
Windows 改用 whpx，macOS 改用 hvf。

系统模拟器机器
--------------

带版本号的机器类型（aarch64、arm、i386、m68k、ppc64、s390x、x86_64）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

按照版本化机器弃用规则，版本不高于 |VER_MACHINE_DELETION_VERSION| 的全部类型已移除。

``s390-virtio`` （2.6 移除）
''''''''''''''''''''''''''''

改用 s390-ccw-virtio。

m68k 的 ``dummy`` 机器（2.9 移除）
''''''''''''''''''''''''''''''''''

改用 none 机器配合 loader 设备。

``xlnx-ep108`` （3.0 移除）
'''''''''''''''''''''''''''

EP108 是不再使用的早期开发板，改用 xlnx-zcu102。

``spike_v1.9.1`` 和 ``spike_v1.10`` （5.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''

版本专用机器被通用 spike 取代。
需要旧规范时，可通过 ``-cpu rv64gcsu,priv_spec=v1.10.0`` 指定。

MIPS ``r4k`` 平台（5.2 移除）
'''''''''''''''''''''''''''''

该机器陈旧且无人维护，改用 malta。

MIPS ``fulong2e`` 别名（6.0 移除）
''''''''''''''''''''''''''''''''''

机器已改名为 fuloong2e。

Raspberry Pi ``raspi2`` 和 ``raspi3`` （6.2 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''

树莓派有 A、A+、B、B+ 等多种型号。
为明确 QEMU 实现的具体型号，二者改名为 raspi2b 和 raspi3b。

Aspeed ``swift-bmc`` （7.0 移除）
'''''''''''''''''''''''''''''''''

没有用户，故移除。基于 AST2500 的替代 OpenPOWER 机器包括 witherspoon-bmc 和 romulus-bmc。

PPC ``taihu`` （7.2 移除）
''''''''''''''''''''''''''

该机器仅部分模拟，且与其他 405 机器相近，改用 ref405ep。

Nios II ``10m50-ghrd`` 和 ``nios2-generic-nommu`` （9.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Nios II 架构无人维护。

``shix`` （9.2 移除）
'''''''''''''''''''''

该机器无人维护。

Arm 机器 ``akita``、``borzoi``、``cheetah``、``connex``、``mainstone``、``n800``、``n810``、``spitz``、``terrier``、``tosa``、``verdex``、``z2`` （9.2 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

这些机器的 SoC 模拟代码陈旧且无人维护，阻碍了代码库的多项改进，
多年来也无人愿意将其现代化。它们模拟的硬件早已过时，预计用户不多。
因此移除所有基于 PXA2xx 和 OMAP2 的机器。
OMAP1 的 cheetah 也被移除，因为没有测试镜像，也不知道有谁持有可用镜像。

Aspeed ``tacoma-bmc`` （10.0 移除）
'''''''''''''''''''''''''''''''''''

相对 rainier-bmc，它没有提供多少额外价值。
tacoma-bmc 是仅用于实验室 AST2600 调试的开发板，从未正式发布。
可用最终产品 rainier-bmc，或稍作调整的 ast2600-evb 替代。

PPC ``ref405ep`` （10.0 移除）
''''''''''''''''''''''''''''''

PPC 405 没有已知用户，也没有可用固件。
OpenWrt 在 2019 年、U-Boot 在 2017 年、Linux 在 2024 年先后停止支持，因此移除。

``petalogix-ml605`` 和 ``xlnx-zynqmp-pmu`` 的大端变体（10.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

这两个 MicroBlaze 机器最初都面向小端 CPU。
大端支持从未测试，很可能从未工作。从 10.1 起只提供小端机器。

MIPS ``mipssim`` （10.2 移除）
''''''''''''''''''''''''''''''

Linux 3.7 已停止支持此虚拟机器，网上也没有可用的二进制镜像。

Arm ``ast2700a0-evb`` （11.0 移除）
'''''''''''''''''''''''''''''''''''

A0 是 AST2700 首版工程样片，不是量产版本。
现在已支持 A1，应改用 ast2700a1-evb。

Arm ``highbank`` 和 ``midway`` （11.0 移除）
''''''''''''''''''''''''''''''''''''''''''''

没有已知用户。如果只想启动 Cortex-A15 或 Cortex-A9 Linux，应使用 virt。

Arm ``sonorapass-bmc`` （11.1 移除）
''''''''''''''''''''''''''''''''''''

该机器模拟从未量产的实验室服务器。
可用 ast2500-evb，通过 fmc-model 指定闪存类型，再从命令行定义 I2C 设备。

Arm ``qcom-dc-scm-v1-bmc`` 和 ``qcom-firework-bmc`` （11.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

二者都是从未量产的实验室服务器，也不依赖专用设备模型。
可用 ast2600-evb，通过 fmc-model 指定闪存类型，再从命令行定义 I2C 设备。

Arm ``fp5280g2-bmc`` （11.1 移除）
''''''''''''''''''''''''''''''''''

该机器不依赖专用设备模型，可用 ast2500-evb，
通过 fmc-model 指定闪存类型，再从命令行定义 I2C 设备。

Arm ``fby35`` （11.1 移除）
'''''''''''''''''''''''''''

该机器最初用于展示多 SoC 系统，预期逐渐发展为异构系统，但未实现，
也没有可供启动的公开固件。
可用基于较新 AST2700 的多 SoC 机器 ast2700fc 替代，后者预计会获得更好的后续支持。

RISC-V 默认机器（11.1 移除）
''''''''''''''''''''''''''''

过去未指定机器时默认使用 spike，因为它是最早实现的 RISC-V 机器，当时这样设置方便。
如今已有 7 种 riscv64 和 6 种 riscv32 机器，不再适合默认选择 spike。

现已取消默认值，用户必须显式选择机器，避免混淆。
原 spike 用户应确保命令行包含 ``-M spike``。

linux-user 模式 CPU
-------------------

``tilegx`` CPU（6.0 移除）
''''''''''''''''''''''''''

tilegx 仅在 linux-user 实现。上游 Linux 于 2018 年移除了该 CPU，glibc 也停止支持，
没有新的 Linux 开发工作，因此移除且无替代项。运行旧程序可使用旧 QEMU。

``ppc64abi32`` CPU（7.0 移除）
''''''''''''''''''''''''''''''

该架构多项问题经常导致 CI 失败，疑似功能已经严重损坏，
维护者因此认为它很可能没有实际用户。

``nios2`` CPU（9.1 移除）
'''''''''''''''''''''''''

QEMU 中该架构无人维护，Intel 也终止了 Nios II IP，见 `Intel discontinuance notification`_。

iwMMXt 模拟及 ``pxa`` CPU（10.2 移除）
''''''''''''''''''''''''''''''''''''''

pxa 系列包括 pxa250、pxa255、pxa260、pxa261、pxa262、pxa270-a0、pxa270-a1、
pxa270、pxa270-b0、pxa270-b1、pxa270-c0 和 pxa270-c5。
使用它们的系统机器已在 9.2 全部移除，因此系统模式已不可用。
需要显式启用的 iwMMXt 模拟也被认为没有用户，
且没有测试、真实硬件或可信实现用于验证，因此用户模式下也移除这些 CPU。

TCG 内省功能
------------

TCG trace-events（7.0 移除）
''''''''''''''''''''''''''''

添加新 TCG 跟踪点的能力长期缺少维护，而且可以通过 TCG 插件实现，因此移除。
需要迁移帮助的用户可联系 qemu-devel 邮件列表。

固件、ACPI 和设备树
-------------------

RISC-V virt 的 riscv,delegate 设备树属性（11.0 移除）
'''''''''''''''''''''''''''''''''''''''''''''''''''''

该属性随 7.0 的 AIA APLIC 支持加入，但在 Linux 审阅过程中改名，
最终正确名称为 riscv,delegation。

系统模拟器设备
--------------

``spapr-pci-vfio-host-bridge`` （2.12 移除）
''''''''''''''''''''''''''''''''''''''''''''

改用 spapr-pci-host-bridge。

``ivshmem`` （4.0 移除）
''''''''''''''''''''''''

改用 ivshmem-plain 或 ivshmem-doorbell。

``ide-drive`` （6.0 移除）
''''''''''''''''''''''''''

按需使用 ide-hd 或 ide-cd，分别获得 IDE 硬盘或光驱。

``scsi-disk`` （6.0 移除）
''''''''''''''''''''''''''

按需使用 scsi-hd 或 scsi-cd，分别获得 SCSI 硬盘或光驱。

``sga`` （8.0 移除）
''''''''''''''''''''

sga 为 x86 加载选项 ROM，使 SeaBIOS 将消息发往串口。
SeaBIOS 1.11.0 起已原生支持，可用 ``-machine graphics=off`` 启用，旧方式不再需要。

``pvrdma`` 和 RDMA 子系统（9.1 移除）
'''''''''''''''''''''''''''''''''''''

pvrdma 设备及整个 RDMA 子系统均已移除。

``-device sd-card,spec_version=1`` （10.2 移除）
''''''''''''''''''''''''''''''''''''''''''''''''

SD 物理层规范 v2.00 已取代 v1.10。

系统模拟器二进制程序
--------------------

``qemu-system-microblazeel`` （11.0 移除）
''''''''''''''''''''''''''''''''''''''''''

qemu-system-microblaze 现在也可以模拟小端机器，无需单独的 el 后缀程序。
petalogix-s3adsp1800 可以将 endianness 属性设为 little，切换到小端。

相关二进制程序
--------------

``qemu-nbd --partition`` （5.0 移除）
'''''''''''''''''''''''''''''''''''''

此选项也写作 -P，只能处理 MBR，且无法正确处理编号超过 5 的逻辑分区。
仍可通过 --image-opts，在其他块设备上叠加带 offset 和 size 的 raw 节点来导出分区。
例如，分区 1 从 1 MiB 开始，长 100 MiB，原命令：::

  qemu-nbd -t -P 1 -f qcow2 file.qcow2


可以改写为：::

  qemu-nbd -t --image-opts driver=raw,offset=1M,size=100M,file.driver=qcow2,file.file.driver=file,file.file.filename=file.qcow2


``qemu-img convert -n -o`` （5.1 移除）
'''''''''''''''''''''''''''''''''''''''

-o 中全部选项都用于创建镜像，因此与跳过创建的 -n 同时使用时会被拒绝。

``qemu-img create -b bad file $size`` （5.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''''''

过去，打不开后备文件时，只要显式提供大小，create 就仅警告而继续创建。
既然已有 -u 专门用于此目的，未指定 -u 时应把任何打开失败当作错误，
包括后备文件缺失或格式错误，这样更安全。

用 ``qemu-img amend`` 修改后备文件（6.1 移除）
''''''''''''''''''''''''''''''''''''''''''''''

此用法从未完整文档化或测试，而且会干扰需要访问原后备镜像的 amend 操作，
例如转换 v3 到 v2 时判断零簇能否保持未分配。
应在其他 amend 修改之前或之后，按需要使用 ``qemu-img rebase -u`` 改变后备链。

``qemu-img`` 的后备文件未指定格式（6.1 移除）
'''''''''''''''''''''''''''''''''''''''''''''

create、rebase 或 convert 创建或修改依赖后备文件的镜像时，
现在必须显式指定后备格式。
如果自动探测出的格式与预期不同，客户机看到的数据会损坏；
将 raw 镜像交给客户机后，如果未来探测因客户机写入而判断为其他格式，也可能产生安全问题。

必须用 ``-o backing_fmt=``，或 create 的简写 -F，指定预期格式。
可以用 ``qemu-img rebase -u`` 为已有镜像补充格式。
但决定格式时，盲目用 qemu-img info 探测不可信后备镜像本身也有潜在风险。

块设备
--------

VXHS 后端（5.1 移除）
'''''''''''''''''''''

VXHS 自 2.12.0 起无法编译，在 5.1 移除。

``sheepdog`` 驱动（6.0 移除）
'''''''''''''''''''''''''''''

上游服务器项目已无人维护，建议切换到 RBD 等其他分布式块设备驱动。

``gluster`` 后端（11.1 移除）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

根据 https://marc.info/?l=fedora-devel-list&m=171934833215726，
GlusterFS 开发实际上已经结束。

VFIO 设备
---------

``-device vfio-calxeda-xgmac`` （10.2 移除）
''''''''''''''''''''''''''''''''''''''''''''

该设备用于将宿主 Calxeda Highbank 10Gb XGMAC 以太网控制器分配给客户机，
兼容字符串为 calxeda,hb-xgmac。相关硬件已淘汰，不再需要保留。

``-device vfio-amd-xgbe`` （10.2 移除）
'''''''''''''''''''''''''''''''''''''''

该设备用于分配宿主 AMD 10GbE 控制器，兼容字符串为 amd,xgbe-seattle-v1a。
AMD Seattle 已不再受支持，因此移除。

``-device vfio-platform`` （10.2 移除）
'''''''''''''''''''''''''''''''''''''''

该设备提供通用的平台设备直通。
新增设备需要同时适配内核和 QEMU，多年来没有此类工作，说明未获得实际采用。
主流方案应采用 PCIe 直通。

工具
--------

virtiofsd（8.0 移除）
'''''''''''''''''''''

新的 Rust 实现位于 ``https://gitlab.com/virtio-fs/virtiofsd``，
已稳定并广泛使用，命令行和功能与被移除的 C 实现十分接近。

QEMU 客户机代理
---------------

``--blacklist`` 命令行选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''

改名为更准确的 --block-rpcs。简写 -b 保持不变，
需要兼容新旧 QEMU 的脚本优先使用 -b。

``blacklist`` 配置文件选项（9.1 移除）
''''''''''''''''''''''''''''''''''''''

改名为 block-rpcs，与对应命令行选项一致。

设备选项
--------

字符设备选项
''''''''''''

``reconnect`` （10.2 移除）
^^^^^^^^^^^^^^^^^^^^^^^^^^^

由精度更高的 reconnect-ms 取代。

网络设备选项
''''''''''''

Stream 的 ``reconnect`` （10.2 移除）
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

由精度更高的 reconnect-ms 取代。

.. _Intel discontinuance notification: https://www.intel.com/content/www/us/en/content-details/781327/intel-is-discontinuing-ip-ordering-codes-listed-in-pdn2312-for-nios-ii-ip.html
