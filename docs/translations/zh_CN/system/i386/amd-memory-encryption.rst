.. _amd-sev:

AMD 安全加密虚拟化（SEV）
=========================================

安全加密虚拟化（SEV）是 AMD 处理器提供的一项功能。

SEV 是 AMD-V 架构的扩展，支持在 KVM 控制下运行加密虚拟机（VM）。加密虚拟机的页面（代码和数据）受到保护，只有客户机自身能够访问未加密的版本。每个加密虚拟机都关联一个唯一的加密密钥；如果其他实体使用不同的密钥访问其数据，就无法正确解密加密客户机的数据，得到的内容也就无法理解。

此功能的密钥管理由 AMD SoC 中一个独立的处理器负责，称为 AMD 安全处理器（AMD-SP）。在 AMD-SP 内运行的固件提供支持常见虚拟机生命周期的命令，包括启动、快照、迁移和调试加密客户机。这些 SEV 命令可以通过 KVM_MEMORY_ENCRYPT_OP ioctl 发出。

安全加密虚拟化——加密状态（SEV-ES）在 SEV 支持的基础上进一步保护客户机寄存器状态。为使虚拟机监控器能够代表客户机执行功能，架构提供了一种机制，在即将发生某些类型的 VMEXIT 时通知客户机操作系统。这让客户机可以选择性地与虚拟机监控器共享信息，以完成所请求的功能。

启动（SEV 和 SEV-ES）
--------------------------

启动客户机前，必须先加密启动镜像（例如 BIOS）。``MEMORY_ENCRYPT_OP`` ioctl 提供了加密镜像的命令：``LAUNCH_START``、``LAUNCH_UPDATE_DATA``、``LAUNCH_MEASURE`` 和 ``LAUNCH_FINISH``。这四个命令共同为虚拟机生成新的内存加密密钥，加密启动镜像，并提供可用于证明启动成功的度量值。

对于 SEV-ES 客户机，还会使用 ``LAUNCH_UPDATE_VMSA`` 命令加密客户机所有 vCPU 的寄存器状态，即虚拟机保存区（VMSA）。

首先调用 ``LAUNCH_START`` 在固件中创建加密启动上下文。为创建此上下文，客户机所有者必须提供客户机策略、其公开 Diffie-Hellman 密钥（PDH）以及会话参数。这些输入应作为二进制数据块处理，并且必须原样传递给 SEV 固件。

客户机策略以明文传递。虚拟机监控器可以读取它，但不应修改它（修改任何策略位都会导致度量值不正确）。客户机策略是一个 4 字节的数据结构，其中包含若干标志，用来限制可对运行中的 SEV 客户机执行的操作。详情参见 SEV API 规范（[SEVAPI]_）第 3 节和第 6.2 节。

可以通过 ``policy`` 属性提供客户机策略::

  # ${QEMU} \
     sev-guest,id=sev0,policy=0x1...\


设置“要求 SEV-ES”策略位（位 2）会将客户机作为 SEV-ES 客户机启动::

  # ${QEMU} \
     sev-guest,id=sev0,policy=0x5...\


客户机所有者提供的 DH 证书和会话参数用于与客户机所有者建立加密会话，以协商用于证明的密钥。

可以通过 ``dh-cert-file`` 和 ``session-file`` 属性提供 DH 证书和会话数据块::

  # ${QEMU} \
       sev-guest,id=sev0,dh-cert-file=<file1>,session-file=<file2>


``LAUNCH_UPDATE_DATA`` 使用 ``LAUNCH_START`` 命令创建的加密上下文对内存区域进行加密。如果需要，可以多次调用此命令来加密不同的内存区域。命令还会在加密过程中计算内存内容的度量值。

``LAUNCH_UPDATE_VMSA`` 使用 ``LAUNCH_START`` 命令创建的加密上下文，对 SEV-ES 客户机的所有 vCPU VMSA 进行加密。命令还会在加密过程中计算 VMSA 的度量值。

``LAUNCH_MEASURE`` 可用于获取加密内存的度量值，以及 SEV-ES 客户机加密 VMSA 的度量值。此度量值是内存内容和（对于 SEV-ES 客户机）VMSA 内容的签名，可以发送给客户机所有者，证明固件正确加密了内存和 VMSA。客户机所有者可以等到验证此证明度量值后，再向客户机提供机密信息。由于客户机所有者知道客户机启动时的初始内容，因此可以将证明度量值与预期值进行比较来验证它。

``LAUNCH_FINISH`` 完成客户机启动并销毁加密上下文。

完整流程图参见 SEV API 规范（[SEVAPI]_）中的“启动客户机”使用流程（附录 A）。

启动 SEV 客户机::

  # ${QEMU} \
      -machine ...,confidential-guest-support=sev0 \
      -object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=1


启动 SEV-ES 客户机::

  # ${QEMU} \
      -machine ...,confidential-guest-support=sev0 \
      -object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=1,policy=0x5


与 SEV 客户机相比，SEV-ES 客户机存在一些限制。由于客户机寄存器状态已加密，VMM/虚拟机监控器无法更新它，因此 SEV-ES 客户机：

 - 不支持 SMM：SMM 支持需要更新客户机寄存器状态。
 - 不支持重启：系统复位需要更新客户机寄存器状态。
 - 要求内核内 irqchip：由虚拟机监控器负责管理 AP 的启动。

计算预期的客户机启动度量值
---------------------------------------------

为验证客户机启动度量值，客户机所有者必须使用与 AMD-SP 完全相同的方法进行计算。SEV API 规范（[SEVAPI]_）第 6.5.1 节描述了 AMD-SP 的操作：

    完成 GCTX.LD 的计算，产生导入客户机的全部明文数据的哈希摘要。

    启动度量值计算如下：

    HMAC(0x04 || API_MAJOR || API_MINOR || BUILD || GCTX.POLICY || GCTX.LD || MNONCE; GCTX.TIK)

    其中“||”表示拼接。

API_MAJOR、API_MINOR、BUILD 和 GCTX.POLICY 的值可通过 ``query-sev`` QMP 命令获取。

MNONCE 的值是 ``query-sev-launch-measure`` 响应的一部分：它是对 data 字段进行 base64 解码后的最后 16 字节（参见 SEV API 规范（[SEVAPI]_）第 6.5.2 节表 52：LAUNCH_MEASURE 度量缓冲区）。

GCTX.LD 的值为 ``SHA256(firmware_blob || kernel_hashes_blob || vmsas_blob)``，其中：

* ``firmware_blob`` 是整个固件闪存文件（例如 ``OVMF.fd``）的内容。注意，必须构建不使用 NVRAM 存储的无状态固件文件，因为 NVRAM 区域不参与度量，因此使用依赖 NVRAM 存储状态的固件是不安全的。
* 如果使用内核并设置 ``kernel-hashes=on``，则 ``kernel_hashes_blob`` 是 PaddedSevHashTable 的内容（包括零填充），其中包含传递给客户机的内核、initrd 和 cmdline 的哈希。PaddedSevHashTable 结构定义在 ``target/i386/sev.c`` 中。
* 如果启用 SEV-ES（``policy & 0x4 != 0``），则 ``vmsas_blob`` 是客户机所有 vCPU VMSA 的拼接。每个 VMSA 长 4096 字节；其内容由 Linux 内核代码中的 ``struct vmcb_save_area`` 定义，也可参见 AMD APM 第 2 卷（[APMVOL2]_）表 B-2：VMCB 布局，状态保存区。

如果不使用内核哈希或未启用 SEV-ES，则按需为 ``kernel_hashes_blob`` 和 ``vmsas_blob`` 使用空数据块。

启动（SEV-SNP）
-------------------

启动客户机前，必须先加密启动镜像（例如 BIOS）。``MEMORY_ENCRYPT_OP`` ioctl 提供了加密镜像的命令：``SNP_LAUNCH_START``、``SNP_LAUNCH_UPDATE`` 和 ``SNP_LAUNCH_FINISH``。这三个命令与 SEV-SNP 固件通信，为虚拟机生成新的内存加密密钥并加密启动镜像，以成功启动客户机。有关这些命令使用的 SEV-SNP 固件接口的更多信息，请参见 SEV-SNP 固件 ABI。

首先调用 ``SNP_LAUNCH_START`` 在固件中创建加密启动上下文。为创建此上下文，客户机所有者必须提供客户机策略和 SEV-SNP 固件规范描述的其他参数。启动参数应按照 sev-snp-guest 对象的 QAPI 模式说明指定。

``SNP_LAUNCH_START`` 使用以下参数，可以通过 'sev-snp-guest' 对象的 QAPI 模式中记录的对应参数进行配置。

+---------------------------+--------+---------+---------------------------------------+
| 键                        | 类型   | 默认值  | 含义                                  |
+---------------------------+--------+---------+---------------------------------------+
| policy                    | hex    | 0x30000 | 64 位客户机策略                       |
+---------------------------+--------+---------+---------------------------------------+
| guest-visible-workarounds | string | 0       | 16 字节的 base64 编码字符串，用于指定 |
|                           |        |         | 客户机操作系统可见的变通措施。        |
+---------------------------+--------+---------+---------------------------------------+

``SNP_LAUNCH_UPDATE`` 使用 ``SNP_LAUNCH_START`` 命令创建的加密上下文对内存区域进行加密。如果需要，可以多次调用此命令来加密不同的内存区域。命令还会在加密过程中计算内存内容的度量值。

``SNP_LAUNCH_FINISH`` 完成客户机启动流程。在完成启动时，固件还可以选择检查通过 ``SNP_LAUNCH_UPDATE`` 计算出的启动摘要。要执行该检查，用户必须提供 ID 块、身份验证数据块和应包含在证明报告中的主机数据。详情参见 SEV-SNP 规范。

``SNP_LAUNCH_FINISH`` 使用以下参数，可以通过 'sev-snp-guest' 对象的 QAPI 模式中记录的对应参数进行配置。

+--------------------+--------+--------+---------------------------+
| 键                 | 类型   | 默认值 | 含义                      |
+--------------------+--------+--------+---------------------------+
| id-block           | string | none   | base64 编码的 ID 块       |
+--------------------+--------+--------+---------------------------+
| id-auth            | string | none   | base64 编码的身份验证信息 |
+--------------------+--------+--------+---------------------------+
| author-key-enabled | bool   | 0      | 身份验证块包含作者密钥    |
+--------------------+--------+--------+---------------------------+
| host_data          | string | none   | 主机提供的数据            |
+--------------------+--------+--------+---------------------------+

启动 SEV-SNP 客户机（其他参数记录在 'sev-snp-guest' 对象的 QAPI 模式中）::

  # ${QEMU} \
    -machine ...,confidential-guest-support=sev0 \
    -object sev-snp-guest,id=sev0,cbitpos=51,reduced-phys-bits=1



调试
---------

由于 SEV 客户机的内存内容已加密，虚拟机监控器访问客户机内存时会得到密文。如果客户机策略允许调试，虚拟机监控器可以使用 DEBUG_DECRYPT 和 DEBUG_ENCRYPT 命令访问客户机内存区域进行调试。QEMU 尚不支持此功能。

快照/恢复
----------------

待完成

实时迁移
---------------

待完成

参考资料
----------

`AMD 内存加密白皮书
<https://www.amd.com/content/dam/amd/en/documents/epyc-business-docs/white-papers/memory-encryption-white-paper.pdf>`_

.. [SEVAPI] `安全加密虚拟化 API
   <https://www.amd.com/system/files/TechDocs/55766_SEV-KM_API_Specification.pdf>`_

.. [APMVOL2] `AMD64 架构程序员手册第 2 卷：系统编程
   <https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf>`_

KVM Forum 演示文稿：

* `AMD 的虚拟化内存加密（2016）
  <http://www.linux-kvm.org/images/7/74/02x08A-Thomas_Lendacky-AMDs_Virtualizatoin_Memory_Encryption_Technology.pdf>`_
* `通过 SEV-ES 扩展安全加密虚拟化（2018）
  <https://www.linux-kvm.org/images/9/94/Extending-Secure-Encrypted-Virtualization-with-SEV-ES-Thomas-Lendacky-AMD.pdf>`_

`AMD64 架构程序员手册：
<https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf>`_

* SME 位于第 7.10 节
* SEV 位于第 15.34 节
* SEV-ES 位于第 15.35 节
