.. _secret data:

向 QEMU 提供秘密数据
-----------------------------

QEMU 多种对象需要管理员或管理程序提供秘密数据，例如网络块设备密码、解锁
LUKS 密钥材料的口令、远程桌面访问密码。secret 对象提供通用的安全传递机制。

启动时可用 -object secret,...，运行时可用 QMP/HMP 的 object_add。以下
示例采用命令行，但同样适用于监视器。每个 secret 必须有唯一 ID，配置使用
秘密的功能时，以该 ID 引用对象。

不安全：直接传入明文秘密
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**生产环境或多用户主机绝不能这样做。命令行参数通常可在进程列表查看，也常被
监控或错误报告工具收集到日志。QMP/HMP 命令及参数也常记录并附到报告，都会
泄露直接传入的秘密。**

为了方便开发和调试，可以直接在命令行传入：

::

   -object secret,id=secvnc0,data=87539319



也可传入 base64，特别适合包含会干扰参数解析的二进制字符的数据。

::

   -object secret,id=secvnc0,data=ODc1MzkzMTk=,format=base64



**注意：base64 编码不提供任何安全保护。**

通过文件传入明文秘密
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

最简单的安全方式是将秘密保存在文件中：

::

   -object secret,id=secvnc0,file=vnc-password.txt



vnc-password.txt 保存明文。QEMU 将文件视为不透明数据块，使用全部原始内容，
因此若换行不属于秘密，不要误加末尾换行。

有时将文件保存为 base64，由 QEMU 解码为原始字节更方便：

::

   -object secret,id=sec0,file=vnc-password.txt,format=base64



文件通常应使用 0600 或 0400 权限，所有者和组与 QEMU 运行账户一致。
使用 SELinux 等强制访问控制时，应标记为仅允许需要它的那个 QEMU 进程访问，
防止其他进程或用户泄露秘密。

直接传入密文秘密
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

为解决直接传入明文的不安全问题，可配置另一个 secret 作为 AES 解密密钥。

该密钥必须始终通过文件机制提供：

::

   -object secret,id=secmaster,file=masterkey.data,format=base64



masterkey.data 应包含 32 个密码学安全随机字节的 base64 编码，用作 AES-256
主密钥，真实秘密经加密后即可安全地直接传入：

::

   -object secret,id=secvnc0,keyid=secmaster,data=BASE64-CIPHERTEXT,iv=BASE64-IV,format=base64



BASE64-CIPHERTEXT 是用主密钥 AES-256-CBC 加密秘密，再对密文作 base64 编码
的结果。BASE64-IV 是 16 个随机字节的 base64 表示，作为初始化向量。

同一主密钥可加密后续全部秘密，**但每个秘密必须使用不同初始化向量**。

通过 Linux 密钥环传入
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

前述方法跨平台通用。Linux 主机还可使用密钥环：

::

   -object secret_keyring,id=secvnc0,serial=1729



这使 QEMU 从序列号 1729 的密钥环条目读取数据，也可结合 base64：

::

   -object secret_keyring,id=secvnc0,serial=1729,format=base64



或结合主密钥加密：

::

   -object secret_keyring,id=secvnc0,keyid=secmaster,serial=1729,iv=BASE64-IV



推荐实践
~~~~~~~~~~~~~

生产部署推荐先设置主密钥，再将其他直接传入的秘密全部用主密钥加密。

每个 QEMU 实例必须有不同主密钥，并从密码学安全随机源生成。QEMU 关闭后
应立即删除。若通过文件提供，访问规则必须限制为仅该 QEMU 进程可读；也可
通过 Linux 密钥环提供。

各设备后端的秘密都必须由此主密钥加密。

即使 -object 参数或 object_add 命令被收集到日志并附在公开问题单中，
该流程也有助于保护后端秘密。唯一需要严密保护的是主密钥文件。
