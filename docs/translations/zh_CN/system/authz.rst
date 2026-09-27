.. _client authorization:

客户端授权
--------------------

网络后端使用 TLS 证书或 SASL 时，客户端证明身份即可访问。如果身份数据库
仅包含 QEMU 客户端，这可能足够。但数据库通常范围更广，仅认证不足以限制
访问，因此 QEMU 提供认证后执行细粒度授权的灵活机制。

身份提供者
~~~~~~~~~~~~~~~~~~

目前两种认证框架会在完成时产生身份：

 * TLS X.509 证书的可分辨名称。

   TLS 服务器可使用不同凭据，最常见是 X.509。最简单配置只为服务器签发
   证书，让客户端防范针对目标服务器的中间人攻击。

   也可要求客户端提供证书，实现双向验证。将 tls-creds-x509 的
   verify-peer=yes 即可，这也是默认值。

   启用对端验证后，客户端证书需由与服务器相同的 CA 签发。如果仍不够
   严格，可进一步用证书可分辨名称作为 QEMU 授权身份。

 * SASL 用户名。

   SASL 认证成功后会提供用户名，格式取决于机制。可能是普通 UNIX 风格的
   joebloggs，也可能在 Kerberos/GSSAPI 中带领域，如 joebloggs@QEMU.ORG。
   无论形式如何，都可交给授权框架。

授权驱动
~~~~~~~~~~~~~~~~~~~~~

通用授权框架支持用户可定制的驱动，以对象形式提供。启动时用 -object，
运行时用 object_add 创建。

简单匹配
^^^^^^^^

此驱动通过与单个身份精确匹配授予访问，适合只允许一个客户端的情况。

例如接收热迁移时，预期来源 QEMU 已知，可匹配其 X.509 身份。或者虚拟机
专供某租户使用，VNC 启用 SASL 并仅列出该租户用户名。

通过 QMP 创建：

::

   {
     "execute": "object-add",
     "arguments": {
       "qom-type": "authz-simple",
       "id": "authz0",
       "identity": "fred"
     }
   }



或命令行：

::

   -object authz-simple,id=authz0,identity=fred



列表
^^^^

需要允许多个客户端时，可用列表规则匹配身份。每条规则有对应策略，无规则
命中时采用默认策略。支持精确字符串或 Shell 风格 glob 通配匹配。

通过 QMP 创建：

::

   {
     "execute": "object-add",
     "arguments": {
       "qom-type": "authz-list",
       "id": "authz0",
       "rules": [
          { "match": "fred", "policy": "allow", "format": "exact" },
          { "match": "bob", "policy": "allow", "format": "exact" },
          { "match": "danb", "policy": "deny", "format": "exact" },
          { "match": "dan*", "policy": "allow", "format": "glob" }
       ],
       "policy": "deny"
     }
   }



由于需要设置嵌套属性，命令行必须采用 -object 的 JSON 语法。不过，多数
情况下下一种驱动更适合。

列表文件
^^^^^^^^^

这是列表驱动的变体，将规则放在独立文件，变更后可自动重载，使策略更动态。

通过 QMP 创建：

::

   {
     "execute": "object-add",
     "arguments": {
       "qom-type": "authz-list-file",
       "id": "authz0",
       "filename": "/etc/qemu/myvm-vnc.acl",
       "refresh": true
     }
   }



refresh 为 yes 时，用 inotify 监视文件变化并自动重载。

myvm-vnc.acl 格式与前一驱动接近：

::

   {
     "rules": [
       { "match": "fred", "policy": "allow", "format": "exact" },
       { "match": "bob", "policy": "allow", "format": "exact" },
       { "match": "danb", "policy": "deny", "format": "exact" },
       { "match": "dan*", "policy": "allow", "format": "glob" }
     ],
     "policy": "deny"
   }



命令行创建：

::

   -object authz-list-file,id=authz0,\
           filename=/etc/qemu/myvm-vnc.acl,refresh=on



PAM
^^^

需要接入 QEMU 外部授权机制时，可使用 PAM 驱动以获得最大灵活性。

通过 QMP 创建：

::

   {
     "execute": "object-add",
     "arguments": {
       "qom-type": "authz-pam",
       "id": "authz0",
       "parameters": {
         "service": "qemu-vnc-tls"
       }
     }
   }



驱动只使用 PAM account 验证子系统。上例需要 /etc/pam.d/qemu-vnc-tls。
简单的文件查找配置可以是：

::

   account requisite  pam_listfile.so item=user sense=allow \
           file=/etc/qemu/vnc.allow



外部文件保存用户名列表。若以 X.509 证书作为用户名，条目应匹配可分辨名称：

::

   CN=laptop.berrange.com,O=Berrange Home,L=London,ST=London,C=GB



命令行创建：

::

   -object authz-pam,id=authz0,service=qemu-vnc-tls



还有许多未在此列出的 PAM 插件，也可以使用 PAM API 开发新插件。

连接后端
~~~~~~~~~~~~~~~~~~~

用 -object 创建授权驱动后，还需关联网络服务。对象有唯一 ID，服务引用此 ID。

设置的属性随身份类型不同而变化。按惯例，TLS 后端提供 tls-authz，SASL
后端提供 sasl-authz。

VNC 使用 SASL 和授权的示例：

::

   $QEMU --object authz-simple,id=authz0,identity=fred \
         --vnc 0.0.0.0:1,sasl,sasl-authz=authz0


同时验证 X.509 证书和 SASL 用户名：

::

   echo "CN=laptop.qemu.org,O=QEMU Project,L=London,ST=London,C=GB" >> tls.acl
   $QEMU --object authz-simple,id=authz0,identity=fred \
         --object authz-list-file,id=authz1,filename=tls.acl \
	 --object tls-creds-x509,id=tls0,dir=/etc/qemu/tls,verify-peer=yes \
         --vnc 0.0.0.0:1,sasl,sasl-authz=auth0,tls-creds=tls0,tls-authz=authz1
