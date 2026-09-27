除普通镜像文件外，模拟存储还可使用 iSCSI 等网络资源，采用特殊 URL 语法指定。

``iSCSI``
   QEMU 可直接访问 iSCSI 资源，作为客户机存储镜像，支持磁盘和 CD-ROM。

   LUN 语法为 "iscsi://<target-ip>[:<port>]/<target-iqn>/<lun>"。

   默认发起方名称为 'iqn.2008-11.org.linux-kvm[:<name>]'，也可通过命令行
   或配置文件设置。

   自 QEMU 2.4 起，可设置请求超时，检测停滞并强制重建会话。单位为秒，
   默认 0 表示不超时，需要 Libiscsi 1.15.0 或更新版本。

   无认证示例：

   .. parsed-literal::

      |qemu_system| -iscsi initiator-name=iqn.2001-04.com.example:my-initiator \\
                       -cdrom iscsi://192.0.2.1/iqn.2001-04.com.example/2 \\
                       -drive file=iscsi://192.0.2.1/iqn.2001-04.com.example/1


   通过 URL 指定 CHAP 用户名和密码：

   .. parsed-literal::

      |qemu_system| -drive file=iscsi://user%password\@192.0.2.1/iqn.2001-04.com.example/1


   通过环境变量指定 CHAP 用户名和密码：

   .. parsed-literal::

      LIBISCSI_CHAP_USERNAME="user" \\
      LIBISCSI_CHAP_PASSWORD="password" \\
      |qemu_system| -drive file=iscsi://192.0.2.1/iqn.2001-04.com.example/1


``NBD``
   QEMU 支持通过 TCP 或 Unix 域套接字访问网络块设备。TCP 默认端口为 10809。

   推荐的 TCP URI 形式为 "nbd://<server-ip>[:<port>]/[<export>]"。

   Unix 域套接字形式为 "nbd+unix:///[<export>]?socket=<domain-socket>"。
   注意 ? 是 Shell 通配符，可能需要引号。

   也支持旧式 TCP 语法："nbd:<server-ip>:<port>[:exportname=<export>]"。

   旧式 Unix 域套接字语法："nbd:unix:<domain-socket>[:exportname=<export>]"。

   TCP 示例：

   .. parsed-literal::

      |qemu_system| --drive file=nbd:192.0.2.1:30000


   Unix 域套接字示例：

   .. parsed-literal::

      |qemu_system| --drive file=nbd:unix:/tmp/nbd-socket


``SSH``
   QEMU 支持通过 SSH 访问远程磁盘。

   示例：

   .. parsed-literal::

      |qemu_system| -drive file=ssh://user\@host/path/to/disk.img
      |qemu_system| -drive file.driver=ssh,file.user=user,file.host=host,file.port=22,file.path=/path/to/disk.img


   当前必须通过 ssh-agent 认证，未来可能支持其他方式。

``HTTP/HTTPS/FTP/FTPS``
   QEMU 支持通过 http(s) 和 ftp(s) 只读访问文件。

   单文件名语法：

   ::

      <protocol>://[<username>[:<password>]@]<host>/<path>


   各部分含义：

   ``protocol``
      http、https、ftp 或 ftps。

   ``username``
      可选，远程服务器认证用户名。

   ``password``
      可选，远程服务器认证密码。

   ``host``
      远程服务器地址。

   ``path``
      远程路径，包含查询字符串。

   还支持：

   ``url``
      显式向驱动传入选项时使用的完整 URL。

   ``readahead``
      每次远程范围请求预读的数据量。可用 T、G、M、K、k、b 后缀，
      无后缀按字节解释，必须为 512 字节的倍数，默认 256k。

   ``sslverify``
      SSL 连接时是否验证远程证书，可为 on 或 off，默认 on。

   ``cookie``
      每次请求发送的 cookie，也可为分号分隔的列表。仅对 HTTP 等支持
      cookie 的协议有效，否则忽略。

   ``timeout``
      CURL 等待服务器响应、获得待下载镜像大小的超时，单位为秒，默认 5 秒。

   ``force-range``
      不使用 HEAD 检测 HTTP 服务器是否支持范围请求，仅使用 GET。
      对不允许 HEAD 的 S3 预签名 URL 尤其有用，默认 false。

   显式传入选项时，driver 的值就是 <protocol>。

   从远程 Fedora 20 live ISO 启动：

   .. parsed-literal::

      |qemu_system_x86| --drive media=cdrom,file=https://archives.fedoraproject.org/pub/archive/fedora/linux/releases/20/Live/x86_64/Fedora-Live-Desktop-x86_64-20-1.iso,readonly

      |qemu_system_x86| --drive media=cdrom,file.driver=http,file.url=http://archives.fedoraproject.org/pub/fedora/linux/releases/20/Live/x86_64/Fedora-Live-Desktop-x86_64-20-1.iso,readonly


   从远程 Fedora 20 云镜像启动，使用本地写入覆盖层、读取时复制和 64k 预读：

   .. parsed-literal::

      qemu-img create -f qcow2 -o backing_file='json:{"file.driver":"http",, "file.url":"http://archives.fedoraproject.org/pub/archive/fedora/linux/releases/20/Images/x86_64/Fedora-x86_64-20-20131211.1-sda.qcow2",, "file.readahead":"64k"}' /tmp/Fedora-x86_64-20-20131211.1-sda.qcow2

      |qemu_system_x86| -drive file=/tmp/Fedora-x86_64-20-20131211.1-sda.qcow2,copy-on-read=on


   从使用自签名证书的 VMware vSphere 服务器镜像启动，使用本地写入覆盖层、
   64k 预读和 10 秒超时：

   .. parsed-literal::

      qemu-img create -f qcow2 -o backing_file='json:{"file.driver":"https",, "file.url":"https://user:password@vsphere.example.com/folder/test/test-flat.vmdk?dcPath=Datacenter&dsName=datastore1",, "file.sslverify":"off",, "file.readahead":"64k",, "file.timeout":10}' /tmp/test.qcow2

      |qemu_system_x86| -drive file=/tmp/test.qcow2
