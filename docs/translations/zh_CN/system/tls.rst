.. _network_005ftls:

为网络服务配置 TLS
------------------------------

QEMU 几乎所有网络服务都支持 TLS 会话加密，以及基于 X.509 的简单客户端
认证。以下介绍证书生成方法，适用于 VNC、TCP 字符后端、NBD 客户端和服务器，
以及迁移客户端和服务器。

证书和私钥必须为 PEM。除核心字段外，证书应包含 v3 基本约束、密钥用途、
密钥使用方式和主体备用名称等扩展。

GnuTLS 的 certtool 可方便地生成格式和字段正确的证书与密钥，也可使用证书
管理服务。

至少需要建立 CA，并为各服务器签发证书。若用证书认证客户端，也需为各客户端
签发。

若服务仅面向私有内网，无需商业 CA。自签名 CA 足够，且避免恶意第三方诱骗
外部 CA 错签证书冒充服务，可能更安全。主要例外是 VNC WebSocket 直接面向
远程浏览器，此时商业 CA 可以省去在各浏览器安装私有 CA 的步骤。

.. _tls_cert_file_naming:

证书文件命名
~~~~~~~~~~~~~~~~~~~~~~~

同一主机所有实例共享 TLS 配置时，建议放在 /etc/pki/qemu；非特权用户
使用 $HOME/.pki/qemu。不同子系统需要不同配置时，可使用其子目录。

传统文件名如下：

* ``ca-cert.pem``：客户端和服务器均必需
* ``ca-crl.pem``：仅服务器可选
* ``server-cert.pem``：服务器必需
* ``server-key.pem``：服务器必需
* ``client-cert.pem``：客户端可选
* ``client-key.pem``：客户端可选
* ``dh-params.pem``：服务器可选

自 QEMU 10.2.0 起，支持最多四组额外身份：

* ``server-cert-[IDX].pem``：服务器可选
* ``server-key-[IDX].pem``：服务器可选
* ``client-cert-[IDX].pem``：客户端可选
* ``client-key-[IDX].pem``：客户端可选

IDX 为 0 至 3，遇到首个缺失索引即停止加载。这些文件可替代或补充传统文件。
先加载传统文件，再按索引加载；多个证书均适用于会话时，优先第一个。因此
文件命名会影响会话证书选择。

多组证书用于逐步过渡到不同密码算法。新部署实例之间优先采用更强算法，同时
兼容历史实例的证书。这对从旧操作系统上的旧 QEMU 热迁移尤其有用，因为旧
系统支持的算法可能较少。

下面示例使用传统名称，也可相应改为索引命名。

.. _tls_005fgenerate_005fca:

建立证书颁发机构
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

每个组织或组织单位只需执行一次。首先生成 CA 私钥，它必须严格保密；一旦
泄露，用它签发的全部证书信任链都将失效。

::

   # certtool --generate-privkey > ca-key.pem


自签名证书必需的核心信息是组织名称。将数据写入 ca.info 模板，可避免
certtool 交互提示::

   # cat > ca.info <<EOF
   cn = Name of your organization
   ca
   cert_signing_key
   EOF
   # certtool --generate-self-signed \
              --load-privkey ca-key.pem \
              --template ca.info \
              --outfile ca-cert.pem


ca 设置 v3 基本约束，表明这是 CA；cert_signing_key 设置密钥使用扩展，
表明可签署其他密钥。ca-cert.pem 应复制到需要 TLS VNC 的所有服务端和
客户端。ca-key.pem 只能保存在负责签发证书的主机，不能泄露或复制到其他位置。

.. _tls_005fgenerate_005fserver:

签发服务器证书
~~~~~~~~~~~~~~~~~~~~~~~~~~~

每台服务器需要密钥和证书。连接时将证书发给客户端，由其对照 CA 验证。
核心字段是客户端连接时使用的主机名或 IP，客户端会与证书记录比较，无
匹配项就关闭连接。

建议同时包含完整域名和短主机名。若 IP 固定且客户端可能直接使用，也可
加入，支持 IPv4 和 IPv6。过去 CN 只包含一个主机名，但用它验证已弃用。
现代客户端使用可容纳多个条目的 SAN 扩展；未来可能完全停止使用 CN，
因此强烈建议提供 SAN。

在 CA 主机创建各服务器模板并签发：

::

   # cat > server-hostNNN.info <<EOF
   organization = Name  of your organization
   cn = hostNNN.foo.example.com
   dns_name = hostNNN
   dns_name = hostNNN.foo.example.com
   ip_address = 10.0.1.87
   ip_address = 192.8.0.92
   ip_address = 2620:0:cafe::87
   ip_address = 2001:24::92
   tls_www_server
   signing_key
   EOF
   # certtool --generate-privkey > server-hostNNN-key.pem
   # certtool --generate-certificate \
              --load-ca-certificate ca-cert.pem \
              --load-ca-privkey ca-key.pem \
              --load-privkey server-hostNNN-key.pem \
              --template server-hostNNN.info \
              --outfile server-hostNNN-cert.pem


dns_name 和 ip_address 设置 SAN。tls_www_server 表示服务器用途，虽然
除 VNC WebSocket 外 QEMU 服务并非 HTTP，仍建议设置。signing_key 表示
密钥用于数据会话。

将 server-hostNNN-key.pem 和 server-hostNNN-cert.pem 安全复制到对应主机，
放入 /etc/pki/qemu 并分别改名 server-key.pem、server-cert.pem。
私钥敏感，应使用 0600 防止泄露。

.. _tls_005fgenerate_005fclient:

签发客户端证书
~~~~~~~~~~~~~~~~~~~~~~~~~~~

X.509 凭据默认验证客户端证书，提供简单认证。因此各客户端也需证书，其中
元数据应能在 CA 范围唯一标识客户端，通常包括组织、省州、城市、建筑等。

同样在 CA 主机创建模板并签发：

::

   # cat > client-hostNNN.info <<EOF
   country = GB
   state = London
   locality = City Of London
   organization = Name of your organization
   cn = hostNNN.foo.example.com
   tls_www_client
   signing_key
   EOF
   # certtool --generate-privkey > client-hostNNN-key.pem
   # certtool --generate-certificate \
              --load-ca-certificate ca-cert.pem \
              --load-ca-privkey ca-key.pem \
              --load-privkey client-hostNNN-key.pem \
              --template client-hostNNN.info \
              --outfile client-hostNNN-cert.pem


客户端不需要 SAN，因此没有 dns_name 和 ip_address。tls_www_client 表示
Web 客户端用途，虽然 QEMU 客户端不是 HTTP，仍建议设置。signing_key
表示用于数据会话。

将 client-hostNNN-key.pem 和 client-hostNNN-cert.pem 安全复制到对应主机，
在 /etc/pki/qemu 中分别命名为 client-key.pem、client-cert.pem。
私钥应使用 0600 保护。

同一主机兼任客户端和服务器时，可以用一张证书覆盖两者。迁移和 NBD 中很
常见，例如 QEMU 先接收 TLS 迁移，后来再迁出。合并两种模板字段即可：

::

   # cat > both-hostNNN.info <<EOF
   country = GB
   state = London
   locality = City Of London
   organization = Name of your organization
   cn = hostNNN.foo.example.com
   dns_name = hostNNN
   dns_name = hostNNN.foo.example.com
   ip_address = 10.0.1.87
   ip_address = 192.8.0.92
   ip_address = 2620:0:cafe::87
   ip_address = 2001:24::92
   tls_www_server
   tls_www_client
   signing_key
   EOF
   # certtool --generate-privkey > both-hostNNN-key.pem
   # certtool --generate-certificate \
              --load-ca-certificate ca-cert.pem \
              --load-ca-privkey ca-key.pem \
              --load-privkey both-hostNNN-key.pem \
              --template both-hostNNN.info \
              --outfile both-hostNNN-cert.pem


复制到目标主机时保存两份，分别为 server-cert.pem/server-key.pem 和
client-cert.pem/client-key.pem。

.. _tls_005fcreds_005fsetup:

配置 TLS X.509 凭据
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

标准加载机制是在系统模拟器 --object 中指定 tls-creds-x509。每组凭据通过
id 获得唯一标识，可由 VNC、迁移、NBD、字符设备等多个后端共享。但客户端
和服务器端点凭据必须分别加载。

dir 指定文件目录，其中应按用途放置前述 ca-cert.pem、server-key.pem、
server-cert.pem、client-key.pem、client-cert.pem。

可以在 dh-params.pem 放入预生成 DH 参数，但此功能已弃用，未来会移除。
省略时按 RFC7919 自动协商。

endpoint 指定客户端或服务器用途，决定加载哪些 PEM。

verify 决定是否验证证书，默认启用。客户端检查服务器名称是否匹配 SAN 或
CN；服务器则要求并验证客户端证书。客户端永远不应禁用验证，服务器仅在
有其他认证方式时可禁用，例如 VNC 用 SASL 认证客户端。

加载服务器凭据并验证客户端：

.. parsed-literal::

   |qemu_system| -object tls-creds-x509,id=tls0,dir=/etc/pki/qemu,endpoint=server


加载客户端凭据：

.. parsed-literal::

   |qemu_system| -object tls-creds-x509,id=tls0,dir=/etc/pki/qemu,endpoint=client


支持 TLS 的网络服务均通过 tls-creds 引用凭据对象 ID，例如 VNC：

.. parsed-literal::

   |qemu_system| -vnc 0.0.0.0:0,tls-creds=tls0


.. _tls_005fpsk:

用于后量子密码学的 TLS 证书
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

若 GnuTLS 足够新，并与操作系统密码策略正确集成和配置，QEMU 可在 TLS
服务中以独占或混合模式支持后量子密码学（PQC）。

独占模式仅配置一组采用 PQC 算法的证书，只能与同样启用 PQC 的服务互操作，
包括其他 QEMU。因此过渡期间可能有兼容性问题。

混合模式配置多组证书，至少一组传统算法和一组 PQC。握手时，双方支持 PQC
则按 GnuTLS 优先级协商使用；一方不支持则回退传统算法。这有助于过渡兼容，
但可能遭到强制降级攻击。确定双方都支持时，应优先选择独占 PQC。

密钥生成参数
^^^^^^^^^^^^^^^^^^^^^^^^^

生成 PQC 私钥时，向 certtool 传入 --key-type。其他 certtool 命令无需
额外参数，行为由密钥类型决定。

常见算法为 ML-DSA-44、ML-DSA-65、ML-DSA-87。无明确要求时，ML-DSA-65
是合适默认选择。

以前述客户端密钥为例::

   # certtool --generate-privkey --key-type=mldsa65 > client-hostNNN-key.pem


服务器以及根、中间 CA 私钥生成也作相同修改。

混合模式需采用索引证书命名。若多张证书都兼容双方算法，使用第一个匹配项。

因此要优先 PQC，使用无索引文件名，或比传统证书更小的索引。例如 PQC 用
server-cert.pem，传统用 server-cert-0.pem；或者分别用索引 0 和 1。

通过算法优先级强制禁用 PQC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

操作系统已启用 PQC 时，可选地在 tls-creds-x509 的 priority 中覆盖并禁用::

  NO_MLDSA="-SIGN-ML-DSA-65:-SIGN-ML-DSA-44:-SIGN-ML-DSA-87"
  NO_MLKEM="-GROUP-X25519-MLKEM768:-GROUP-SECP256R1-MLKEM768:-GROUP-SECP384R1-MLKEM1024"
  # qemu-nbd --object tls-creds-x509,id=tls0,endpoint=server,dir=....,priority=@SYSTEM:$NO_MLDSA:$NO_MLKEM



TLS 预共享密钥（PSK）
~~~~~~~~~~~~~~~~~~~~~~~~~

也可使用 TLS-PSK 替代证书，设置更简单，但扩展性较弱。

用 GnuTLS psktool 生成包含一个或多个用户名及随机密钥的 keys.psk::

   mkdir -m 0700 /tmp/keys
   psktool -u rich -p /tmp/keys/keys.psk


qemu-nbd 等 TLS 服务器可这样使用目录::

   qemu-nbd \
     -t -x / \
     --object tls-creds-psk,id=tls0,endpoint=server,dir=/tmp/keys \
     --tls-creds tls0 \
     image.qcow2


QEMU 客户端连接时，指定 keys.psk 所在目录和可选用户名，默认 qemu::

   qemu-img info \
     --object tls-creds-psk,id=tls0,dir=/tmp/keys,username=rich,endpoint=client \
     --image-opts \
     file.driver=nbd,file.host=localhost,file.port=10809,file.tls-creds=tls0,file.export=/
