.. _VNC security:

VNC 安全
------------

VNC 服务器通过网络提供客户机图形控制台访问，不同部署场景有不同安全注意事项。

.. _vnc_005fsec_005fnone:

不使用密码
~~~~~~~~~~~~~~~~~

最简单的服务器没有任何认证。建议仅监听 Unix 域套接字，例如：

.. parsed-literal::

   |qemu_system| [...OPTIONS...] -vnc unix:/home/joebloggs/.qemu-myvm-vnc


这样仅本机对该路径有读写权限的用户可以访问。远程安全访问可通过 netcat
和 SSH 建立安全隧道。

.. _vnc_005fsec_005fpassword:

使用密码
~~~~~~~~~~~~~~

VNC 密码认证支持有限，密码最多 8 字符，不应视为高安全性。客户端反复连接
较容易暴力破解，因此只应监听环回接口或 Unix 域套接字。此方式使用 DES，
FIPS 140-2 模式不支持。用 password 选项请求认证，启动后通过监视器设置
密码；设置之前，拒绝所有客户端。

.. parsed-literal::

   |qemu_system| [...OPTIONS...] -vnc :1,password=on -monitor stdio
   (qemu) change vnc password
   Password: ********
   (qemu)


.. _vnc_005fsec_005fcertificate:

使用 X.509 证书
~~~~~~~~~~~~~~~~~~~~~~

QEMU 实现 VeNCrypt 扩展，通过 TLS 加密会话、X.509 认证。强烈建议证书，
因为只有 TLS 而无证书认证易受中间人攻击。基础 X.509 配置提供安全会话，
但不认证客户端，任何客户端都可连接并获得加密连接。

.. parsed-literal::

   |qemu_system| [...OPTIONS...] \
     -object tls-creds-x509,id=tls0,dir=/etc/pki/qemu,endpoint=server,verify-peer=off \
     -vnc :1,tls-creds=tls0 -monitor stdio


/etc/pki/qemu 至少包含 ca-cert.pem、server-cert.pem、server-key.pem。
非特权用户可用 $HOME/.pki/qemu 等私有目录。私钥文件应为 0600，仅所有者可读。

.. _vnc_005fsec_005fcertificate_005fverify:

使用 X.509 并验证客户端
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

服务器也可要求客户端证书，并用 CA 验证。适合使用内部私有 CA 的环境。
语法与前述相同，只需将 verify-peer 改为 on。

.. parsed-literal::

   |qemu_system| [...OPTIONS...] \
     -object tls-creds-x509,id=tls0,dir=/etc/pki/qemu,endpoint=server,verify-peer=on \
     -vnc :1,tls-creds=tls0 -monitor stdio


.. _vnc_005fsec_005fcertificate_005fpw:

使用 X.509、客户端验证和密码
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

还可结合 VNC 密码，为客户端提供两层认证。

.. parsed-literal::

   |qemu_system| [...OPTIONS...] \
     -object tls-creds-x509,id=tls0,dir=/etc/pki/qemu,endpoint=server,verify-peer=on \
     -vnc :1,tls-creds=tls0,password=on -monitor stdio
   (qemu) change vnc password
   Password: ********
   (qemu)


.. _vnc_005fsec_005fsasl:

使用 SASL 认证
~~~~~~~~~~~~~~~~~~~~~~~~

SASL 是可扩展、可插拔的 VNC 认证扩展，可接入 PAM、GSSAPI/Kerberos、LDAP、
SQL 数据库、一次性密钥等。安全强度取决于机制，若机制提供 SSF 层，也会
加密数据流。

后文说明如何选择机制。假定支持 SSF，可这样启动：

.. parsed-literal::

   |qemu_system| [...OPTIONS...] -vnc :1,sasl=on -monitor stdio


.. _vnc_005fsec_005fcertificate_005fsasl:

使用 X.509 和 SASL
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

若所需 SASL 机制不支持 SSF，强烈建议结合 TLS 与 X.509，加密数据流，避免
凭据泄露。将 sasl 与前述 TLS 选项结合即可：

.. parsed-literal::

   |qemu_system| [...OPTIONS...] \
     -object tls-creds-x509,id=tls0,dir=/etc/pki/qemu,endpoint=server,verify-peer=on \
     -vnc :1,tls-creds=tls0,sasl=on -monitor stdio


.. _vnc_005fsetup_005fsasl:

配置 SASL 机制
~~~~~~~~~~~~~~~~~~~~~~~~~~~

下面假定 Linux 主机使用 Cyrus SASL，但原则也适用于其他实现或主机。
启用后，从默认 /etc/sasl2/qemu.conf 加载配置。非特权用户可通过
SASL_CONF_PATH 环境变量指定其他搜索位置。

启用 TLS 时，由它提供会话加密，否则必须由 SASL 提供，可选插件大为减少。
按现代标准，只有 GSSAPI 达到可接受水平。旧文档提过 DIGEST-MD5，但 RFC 6331
详述了多个严重缺陷，已不应使用。SCRAM-SHA-256 提供类似的用户名密码认证，
但不提供会话加密，只能结合 TLS。

不用 TLS 时推荐：

::

   mech_list: gssapi
   keytab: /etc/qemu/krb5.tab


这使用 Kerberos v5 的 GSSAPI，服务器主体保存在 /etc/qemu/krb5.tab。
KDC 管理员需生成 qemu/somehost.example.com@EXAMPLE.COM 主体，将主机名
替换为运行 QEMU 的完整域名，领域替换为实际 Kerberos Realm。

使用 TLS 且需要用户名密码认证时，合理配置为：

::

   mech_list: scram-sha-256
   sasldb_path: /etc/qemu/passwd.db


可用 saslpasswd2 向 passwd.db 添加账户。注意该文件以明文保存密码。

其他配置由读者按需研究。除 GSSAPI 外，所有机制都应结合 TLS 以确保安全通道。
