磁盘镜像文件格式
~~~~~~~~~~~~~~~~~~~~~~~

QEMU 支持多种可供虚拟机和 qemu-img 等工具使用的格式，包括首选 raw、qcow2，
以及为兼容旧版 QEMU 或其他监控程序保留的格式。

可通过 -o 向 qemu-img create 或 convert 传入格式专用选项，以下逐一说明。

.. program:: image-formats
.. option:: raw

  原始磁盘镜像，简单且易于导出到其他模拟器。若文件系统支持稀疏空洞，
  例如 Linux ext2/ext3 或 Windows NTFS，只有写入的扇区占用空间。
  可用 qemu-img info 或 Unix/Linux 的 ls -ls 查看实际占用。

  支持选项：

  .. program:: raw
  .. option:: preallocation

    预分配模式为 off、falloc 或 full。falloc 调用 posix_fallocate()，
    full 通过向底层存储写数据预分配，数据是否为零取决于存储位置。

.. program:: image-formats
.. option:: qcow2

  QEMU 功能最丰富的格式，可生成较小镜像，尤其适用于不支持空洞的文件系统，
  支持 zlib 压缩和多个虚拟机快照。

  支持选项：

  .. program:: qcow2
  .. option:: compat

    选择 qcow2 版本。0.10 使用从 QEMU 0.10 起均可读取的传统格式；
    1.1 为默认，启用仅 QEMU 1.1 及以后识别的扩展，包括零簇等，
    可为稀疏镜像高效实现读取时复制。

  .. option:: backing_file

    基础镜像文件名，见 create 子命令。

  .. option:: backing_fmt

    基础镜像格式。

  .. option:: encryption

    已弃用，等同 encrypt.format=aes。

  .. option:: encrypt.format

    luks 表示以 LUKS 加密 qcow2 有效载荷，不加密头部。解锁密钥槽的口令
    由 encrypt.key-secret 提供，其他 encrypt.* 参数可调节加密配置。

    aes 使用 128 位 AES-CBC，密钥由 encrypt.key-secret 提供。按现代
    密码学标准，此格式存在多个设计缺陷：

    - 初始化向量由扇区号推导，可预测，易受选择明文攻击，泄露加密数据存在性。
    - 用户口令直接作为密钥，短口令或弱口令会破坏安全性。
    - 口令泄露后无法原地修改，必须克隆到使用新口令的文件，再用 shred 等
      安全擦除原文件；但对许多现代存储，擦除也未必有效。

    系统模拟器已不支持此加密，仅命令行工具保留以便恢复数据和兼容旧版本。
    应改用 luks。

  .. option:: encrypt.key-secret

    包含口令（luks）或密钥（aes）的 secret 对象 ID。

  .. option:: encrypt.cipher-alg

    加密算法及密钥长度，当前默认 aes-256，仅用于 luks。

  .. option:: encrypt.cipher-mode

    加密模式，默认 xts，仅用于 luks。

  .. option:: encrypt.ivgen-alg

    初始化向量生成算法，默认 plain64，仅用于 luks。

  .. option:: encrypt.ivgen-hash-alg

    向量生成器所需的哈希算法，默认 sha256，仅用于 luks。

  .. option:: encrypt.hash-alg

    PBKDF 使用的哈希算法，默认 sha256，仅用于 luks。

  .. option:: encrypt.iter-time

    每个密钥槽 PBKDF 的耗时，单位为毫秒，默认 2000，仅用于 luks。

  .. option:: cluster_size

    簇大小，必须在 512 到 2M 之间。较小簇可减小镜像体积，较大簇通常性能更好。

  .. option:: preallocation

    off、metadata、falloc 或 full。预分配元数据使初始镜像较大，但增长时
    性能更好。falloc 和 full 类似 raw 对应模式，同时建立元数据。

  .. option:: lazy_refcounts

    on 延迟引用计数更新，减少元数据 I/O、提高性能，尤其适合不批量更新
    元数据的 :option:`cache=writethrough`。代价是主机崩溃后需重建计数表，
    下次打开自动运行 qemu-img check -r all，可能耗时较长。

    仅 compat=1.1 时可启用。

  .. option:: nocow

    on 禁用文件写时复制，仅对 btrfs 有效。

    btrfs 托管虚拟机镜像时性能可能较低，客户机也使用 btrfs 时更明显。
    禁用 COW 可缓解，通常有两种方式：

    - 以 nodatacow 挂载，使新文件均为 NOCOW。
    - 为空文件添加 NOCOW 属性，本选项采用此方式。

    仅适用于新建或空文件。已有数据块的 COW 文件不能通过此选项转换。
    用 lsattr filename 检查，大写 C 表示 NOCOW。

.. program:: image-formats
.. option:: qed

   旧 QEMU 格式，支持基础镜像和紧凑文件，适用于文件系统或传输介质不支持空洞。

   转为 qcow2 时，可考虑 lazy_refcounts=on，以获得更接近 QED 的行为。

   支持选项：

   .. program:: qed
   .. option:: backing_file

      基础镜像文件名，见 create。

   .. option:: backing_fmt

     可选基础镜像格式，适合某些无头部、无法自动识别的 vhd/vpc 文件。

   .. option:: cluster_size

     簇大小，必须是 4K 到 64K 间的 2 的幂。小簇节省体积，大簇通常性能更好。

   .. option:: table_size

     每个 L1/L2 表的簇数，必须为 1 到 16 间的 2 的幂。通常无需修改，
     可用于性能基准测试。

.. program:: image-formats
.. option:: qcow

  旧 QEMU 格式，支持基础镜像、紧凑文件、加密和压缩。

  支持选项：

   .. program:: qcow
   .. option:: backing_file

     基础镜像文件名，见 create。

   .. option:: encryption

     已弃用，等同 encrypt.format=aes。

   .. option:: encrypt.format

     aes 使用 128 位 AES-CBC，密钥由 encrypt.key-secret 提供，存在前文
     qcow2 部分列出的现代密码学设计缺陷。

     系统模拟器已不支持，仅命令行工具保留以便恢复数据和兼容旧版本。

     需要原生加密时，应使用 qcow2 加 encrypt.format=luks。

   .. option:: encrypt.key-secret

     包含 AES 加密密钥的 secret 对象 ID。

.. program:: image-formats
.. option:: luks

  与 Linux dm-crypt/cryptsetup 兼容的 LUKS v1 加密格式。

  支持选项：

  .. program:: luks
  .. option:: key-secret

    包含口令的 secret 对象 ID。

  .. option:: cipher-alg

    加密算法及密钥长度，当前默认 aes-256。

  .. option:: cipher-mode

    加密模式，默认 xts。

  .. option:: ivgen-alg

    初始化向量生成算法，默认 plain64。

  .. option:: ivgen-hash-alg

    向量生成器所需的哈希算法，默认 sha256。

  .. option:: hash-alg

    PBKDF 哈希算法，默认 sha256。

  .. option:: iter-time

    每密钥槽 PBKDF 的耗时，单位为毫秒，默认 2000。

.. program:: image-formats
.. option:: vdi

  兼容 VirtualBox 1.1 的格式。

  支持选项：

  .. program:: vdi
  .. option:: static

    on 表示创建时预分配元数据。

.. program:: image-formats
.. option:: vmdk

  兼容 VMware 3 和 4 的格式。

  支持选项：

  .. program: vmdk
  .. option:: backing_file

    基础镜像文件名，见 create。

  .. option:: compat6

    创建 VMDK 版本 6，而非版本 4。

  .. option:: hwversion

    虚拟硬件版本，指定后不能同时启用 compat6。

  .. option:: subformat

    子格式可选 monolithicSparse（默认）、monolithicFlat、
    twoGbMaxExtentSparse、twoGbMaxExtentFlat、streamOptimized。

.. program:: image-formats
.. option:: vpc

  VirtualPC 兼容格式（VHD）。

  支持选项：

  .. program:: vpc
  .. option:: subformat

    子格式为 dynamic（默认）或 fixed。

.. program:: image-formats
.. option:: VHDX

  Hyper-V 兼容格式（VHDX）。

  支持选项：

  .. program:: VHDX
  .. option:: subformat

    子格式为 dynamic（默认）或 fixed。

    .. option:: block_state_zero

      强制使用 ZERO 类型载荷块，可为 on（默认）或 off。关闭后，新块为
      PAYLOAD_BLOCK_NOT_PRESENT，解析器可返回任意数据。qemu-img convert
      配合 subformat=dynamic 时，不要关闭。

    .. option:: block_size

      块大小，1 MB 到 256 MB，0 表示根据镜像大小自动计算。

    .. option:: log_size

      日志大小，最小 1 MB。

只读格式
~~~~~~~~~~~~~~~~~

以下其他格式仅支持只读访问。

.. program:: image-formats
.. option:: bochs

  growing 类型 Bochs 镜像。

.. program:: image-formats
.. option:: cloop

  Linux Compressed Loop 镜像，仅用于直接复用 Knoppix 等压缩 CD-ROM 镜像。

.. program:: image-formats
.. option:: dmg

  Apple 磁盘镜像。

.. program:: image-formats
.. option:: parallels

  Parallels 磁盘镜像。

使用主机驱动器
~~~~~~~~~~~~~~~~~

除镜像文件外，也可直接访问主机设备。以下适用于 QEMU 0.8.3 及以后。

Linux
^^^^^

有足够权限时，可以设备文件名替代镜像名，例如 /dev/cdrom。

CD
  未插入光盘也可指定设备。QEMU 能检测插入和移除，支持客户机弹出。
  目前仅支持数据 CD。

Floppy
  未插入软盘也可指定设备，但移除检测不准确：若换盘期间未在空驱状态访问，
  客户机会认为仍是同一张。使用主机软驱已弃用，未来会移除支持。

Hard disks
  可使用硬盘，通常指定整盘 /dev/hdb 而非分区 /dev/hdb1，让客户机看到
  分区磁盘。警告：除非清楚后果，最好只读访问，否则可能损坏主机数据。
  可用 -snapshot 或调整设备权限。

Zoned block devices
  模拟控制器支持分区存储时，可直通 Zoned 块设备。使用
  ``--blockdev host_device,node-name=drive0,filename=/dev/nullb0,cache.direct=on``
  将 /dev/nullb0 作为 drive0。

Windows
^^^^^^^

CD
  首选盘符，例如 d:，也支持 ``\\.\d:``，/dev/cdrom 是第一台光驱别名。

  目前没有专门处理可移动介质的代码，建议用监视器 change 或 eject 操作。

Hard disks
  语法为 ``\\.\PhysicalDriveN``，N 为盘号，从 0 开始。

  警告：除非清楚后果，最好只读，否则可能损坏主机数据。可使用 -snapshot，
  将修改写到临时文件。

Mac OS X
^^^^^^^^

/dev/cdrom 是第一台光驱别名。

目前没有专门处理可移动介质的代码，建议用监视器 change 或 eject 操作。

虚拟 FAT 磁盘镜像
~~~~~~~~~~~~~~~~~~~~~~~

QEMU 可从目录树自动创建虚拟 FAT 镜像：

.. parsed-literal::

  |qemu_system| linux.img -hdb fat:/my_directory


即可访问 /my_directory 中全部文件，无需复制进镜像，也无需 SAMBA 或 NFS。
默认只读。

用 :floppy: 模拟软盘：

.. parsed-literal::

  |qemu_system| linux.img -fda fat:floppy:/my_directory


使用 :rw: 可测试读写支持，目前处于 beta 阶段：

.. parsed-literal::

  |qemu_system| linux.img -fda fat:floppy:rw:/my_directory


绝不要：

- 使用非 ASCII 文件名
- 同时使用 -snapshot 和 :rw:
- 期望它配合 loadvm 正常工作
- 客户机访问时同时在主机修改 FAT 目录

NBD 访问
~~~~~~~~~~

QEMU 可直接访问通过网络块设备协议导出的块设备。

.. parsed-literal::

  |qemu_system| linux.img -hdb nbd://my_nbd_server.mydomain.org:1024/


服务器位于同一主机时，可用 Unix 套接字替代网络套接字：

.. parsed-literal::

  |qemu_system| linux.img -hdb nbd+unix://?socket=/tmp/my_socket


此时用 qemu-nbd 导出：

.. parsed-literal::

  qemu-nbd --socket=/tmp/my_socket my_disk.qcow2


也可让多个客户机共享磁盘：

.. parsed-literal::

  qemu-nbd --socket=/tmp/my_socket --share=2 my_disk.qcow2


随后供两个客户机使用：

.. parsed-literal::

  |qemu_system| linux1.img -hdb nbd+unix://?socket=/tmp/my_socket
  |qemu_system| linux2.img -hdb nbd+unix://?socket=/tmp/my_socket


若服务器使用命名导出（NBD 2.9.18 起或 QEMU 内置服务器支持），URI 中必须
指定导出名称：

.. parsed-literal::

  |qemu_system| -cdrom nbd://localhost/debian-500-ppc-netinst
  |qemu_system| -cdrom nbd://localhost/openSUSE-11.1-ppc-netinst


QEMU 1.3 起支持 URI，也仍支持以下旧语法：

.. parsed-literal::

  |qemu_system| linux.img -hdb nbd:my_nbd_server.mydomain.org:1024
  |qemu_system| linux2.img -hdb nbd:unix:/tmp/my_socket
  |qemu_system| -cdrom nbd:localhost:10809:exportname=debian-500-ppc-netinst


iSCSI LUN
~~~~~~~~~~

iSCSI 是通过网络访问 SCSI 设备的常用协议。QEMU 有两种使用方式。

第一种是在主机挂载 LUN，成为普通 SCSI 设备，再从 QEMU 作为 /dev/sd 访问。
具体操作取决于主机系统。

第二种使用 QEMU 内置发起方，跨主机操作系统保持一致。下文介绍此方式。

iSCSI 设备通过特殊 URL 指定：

::

  iscsi://[<username>[%<password>]@]<host>[:<port>]/<target-iqn-name>/<lun>


用户名和密码可选，仅在目标使用 CHAP 访问控制时需要。也可通过环境变量设置，
避免出现在进程列表：

::

  export LIBISCSI_CHAP_USERNAME=<username>
  export LIBISCSI_CHAP_PASSWORD=<password>
  iscsi://<host>/<target-iqn-name>/<lun>


会话参数可通过特殊选项，在 -readconfig 配置文件或命令行设置。

未指定 initiator-name 时，默认使用 'iqn.2008-11.org.linux-kvm[:<uuid>']，
其中 uuid 是虚拟机 UUID。未指定 UUID 时，用名称代替，即
'iqn.2008-11.org.linux-kvm[:<name>']。

指定登录目标所用发起方名称：

::

  -iscsi initiator-name=iqn.qemu.test:my-initiator


控制与目标协商的头部摘要类型：

::

  -iscsi header-digest=CRC32C|CRC32C-NONE|NONE-CRC32C|NONE


也可通过配置文件：

::

  [iscsi]
    user = "CHAP username"
    password = "CHAP password"
    initiator-name = "iqn.qemu.test:my-initiator"
    # header digest is one of CRC32C|CRC32C-NONE|NONE-CRC32C|NONE
    header-digest = "CRC32C"


指定目标名称，允许为不同目标设置不同选项：

::

  [iscsi "iqn.target.name"]
    user = "CHAP username"
    password = "CHAP password"
    initiator-name = "iqn.qemu.test:my-initiator"
    # header digest is one of CRC32C|CRC32C-NONE|NONE-CRC32C|NONE
    header-digest = "CRC32C"


使用配置文件设置 iSCSI：

.. parsed-literal::

  cat >iscsi.conf <<EOF
  [iscsi]
    user = "me"
    password = "my password"
    initiator-name = "iqn.qemu.test:my-initiator"
    header-digest = "CRC32C"
  EOF

  |qemu_system| -drive file=iscsi://127.0.0.1/iqn.qemu.test/1 \\
    -readconfig iscsi.conf


以下在环回接口建立一个含 CD-ROM 和磁盘的 Linux STGT 目标，并由 QEMU 访问。
Red Hat 系列提供 scsi-target-utils 软件包。

.. parsed-literal::

  tgtd --iscsi portal=127.0.0.1:3260
  tgtadm --lld iscsi --op new --mode target --tid 1 -T iqn.qemu.test
  tgtadm --lld iscsi --mode logicalunit --op new --tid 1 --lun 1 \\
      -b /IMAGES/disk.img --device-type=disk
  tgtadm --lld iscsi --mode logicalunit --op new --tid 1 --lun 2 \\
      -b /IMAGES/cd.iso --device-type=cd
  tgtadm --lld iscsi --op bind --mode target --tid 1 -I ALL

  |qemu_system| -iscsi initiator-name=iqn.qemu.test:my-initiator \\
    -boot d -drive file=iscsi://127.0.0.1/iqn.qemu.test/1 \\
    -cdrom iscsi://127.0.0.1/iqn.qemu.test/2


SSH 磁盘镜像
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

通过 SSH 访问远程服务器镜像：

.. parsed-literal::

  |qemu_system| -drive file=ssh://[USER@]SERVER[:PORT]/PATH[?host_key_check=HOST_KEY_CHECK]


属性形式的替代语法：

.. parsed-literal::

  |qemu_system| -drive file.driver=ssh[,file.user=USER],file.host=SERVER[,file.port=PORT],file.path=PATH[,file.host_key_check=HOST_KEY_CHECK]


ssh 是协议。

USER 是远程用户，省略时尝试本地用户名。

SERVER 是远程 SSH 服务器，必须实现 sftp-server。多数 Unix/Linux 无需
额外配置。

PORT 是 sshd 监听端口，默认为 22。

PATH 是磁盘镜像路径。

可选 HOST_KEY_CHECK 控制主机密钥检查。默认 yes 使用本地 .ssh/known_hosts，
no 禁用检查。也可匹配特定指纹，支持 md5、sha1、sha256，但前两者按现代
标准不安全，强烈建议 sha256。指纹必须为十六进制字符串::

  host_key_check=sha256:04ce2ae89ff4295a6b9c4111640bdcb3297858ee55cb434d9dd88796e93aa795


每对十六进制数字之间可选加冒号。

known_hosts 保存 base64 编码密钥，可用以下命令转为 QEMU 所需格式::

   $ for key in `grep 10.33.8.112 known_hosts | awk '{print $3}'`
     do
       echo $key | base64 -d | sha256sum
     done
     6c3aa525beda9dc83eadfbd7e5ba7d976ecb59575d1633c87cd06ed2ed6e366f  -
     12214fd9ea5b408086f98ecccd9958609bd9ac7c0ea316734006bc7818b45dc8  -
     d36420137bcbd101209ef70c3b15dc07362fbe0fa53c5b135eba6e6afa82f0ce  -


同一主机可能有不同算法的多个密钥，应选择与 QEMU 将协商的算法匹配的指纹。

目前认证必须使用 ssh-agent，未来可能支持其他方式。

许多 SSH 服务器不支持 fsync 类操作，驱动无法保证刷新请求被执行；写入时
服务器或网络故障可能损坏磁盘。不支持时会警告：

::

  warning: ssh server ssh.example.com:22 does not support fsync


足够新的 libssh 和 OpenSSH 支持 fsync。

NVMe 磁盘镜像
~~~~~~~~~~~~~~~~

QEMU 用户空间驱动可直接访问 NVMe，绕过主机文件系统和块层，同时保留块任务、
I/O 限流、镜像格式等功能。性能通常高于通过线程池或 linux-aio 使用
-drive file=/dev/sda。

启动后由 QEMU 独占控制器。要与其他虚拟机或主机应用共享存储，应使用文件协议。

启动前将主机 NVMe 绑定到 vfio-pci，例如：

.. parsed-literal::

  # modprobe vfio-pci
  # lspci -n -s 0000:06:0d.0
  06:0d.0 0401: 1102:0002 (rev 08)
  # echo 0000:06:0d.0 > /sys/bus/pci/devices/0000:06:0d.0/driver/unbind
  # echo 1102 0002 > /sys/bus/pci/drivers/vfio-pci/new_id

  # |qemu_system| -drive file=nvme://HOST:BUS:SLOT.FUNC/NAMESPACE


也可用属性语法：

.. parsed-literal::

  |qemu_system| -drive file.driver=nvme,file.device=HOST:BUS:SLOT.FUNC,file.namespace=NAMESPACE


HOST:BUS:SLOT.FUNC 是主机上控制器的 PCI 地址。

NAMESPACE 是命名空间编号，从 1 开始。

磁盘镜像文件锁
~~~~~~~~~~~~~~~~~~~~~~~

后端协议和主机支持时，QEMU 默认防止意外并发访问。同一镜像被多个 QEMU
模拟器或工具以冲突模式打开时，除第一个外都会报错。

Linux 文件协议目前支持 OFD 文件描述符锁，POSIX 主机不支持时可配置回退到
POSIX 锁。

显式指定 locking=on 可启用。若 OFD 不可用，会警告并采用 POSIX 锁，但
由于该 API 局限，热插拔或块任务期间可能静默丢失锁。

共享存储迁移时自动处理锁移交。多虚拟机共享磁盘应使用 share-rw。

默认客户机独占写入。若能安全与其他写者共享，可用
``-device ...,share-rw=on``，但只有集群文件系统等能协调访问、防止损坏的
软件才安全。

该选项仅声明客户机共享能力，不改变镜像格式等 QEMU 功能对独占写入的要求。

也可用 locking=off 完全禁用。协议节点通常是格式驱动的 file 子节点，因此
命令行常写 file.locking=off。例如：

::

  -blockdev driver=qcow2,file.filename=/path/to/image,file.locking=off,file.driver=file


在主机查看 lslocks，确认 QEMU 是否对镜像持锁。可能锁定多个字节，各字节
表示块驱动获取或保护的一项权限。

过滤驱动
~~~~~~~~~~~~~~

过滤驱动不保存数据，而是拦截 I/O 请求执行额外工作。

.. program:: filter-drivers
.. option:: preallocate

  此过滤器放在格式与协议节点之间。写入超出文件末尾时，扩展底层文件并
  预分配额外空间，适用于分配较慢的文件系统。

  支持选项：

  .. program:: preallocate
  .. option:: prealloc-align

    预分配时将文件长度对齐到此字节数，默认 1M。

  .. program:: preallocate
  .. option:: prealloc-size

    预分配字节数，默认 128M。
