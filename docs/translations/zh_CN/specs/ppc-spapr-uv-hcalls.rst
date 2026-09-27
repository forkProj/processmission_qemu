============================================================
虚拟机管理器调用与 Ultravisor
============================================================

支持 Protected Execution Facility（PEF）的 PPC64 系统可将内存置于安全区域，
只有固件中的 Ultravisor 能提供访问。pSeries 客户机通过 ultracall 与其通信，
切换到安全虚拟机（SVM）模式，将内存迁入该区域，使主机普通进程和其他客户机无法访问。

SVM 相关 ultracall/hypercall 目前仅在内部文档记录，计划纳入 [LoPAR]_。
已提交内部 ACR，为此用途保留调用号范围，避免与 IBM 内部维护的 PAPR+ 规范冲突。
本文概述与 QEMU 有关的细节。

Ultravisor 所需的 hypercall
============================================================

切换 SVM 时，Ultravisor 向虚拟机管理器发出多个 hcall，
协调客户机内存迁入安全内存及其他 SVM 操作。
调用号位于保留范围 0xEF00–0xEF80，以下介绍与 QEMU 相关的调用。

``H_TPM_COMM`` （ ``0xef10`` ）
------------------------------------------------------------

SVM 文件系统由对称密钥加密。该密钥又由可信系统的公钥封装／加密，
私钥保存在系统 TPM 中。Ultravisor 用此 hcall 通过 TPM 或关联 TPM 资源管理器
解封装／解封该对称密钥。

主机启动时，Ultravisor 预先与 TPM 建立独立会话密钥，
全部敏感输入输出均由它加密。虚拟机管理器虽然能看到原始输入输出缓冲区，
敏感内容通常仍是会话密钥加密的数据。

参数：

  ``r3``：``H_TPM_COMM`` （ ``0xef10`` ）

  ``r4``：TPM 操作，可为：

    ``TPM_COMM_OP_EXECUTE`` （ ``0x1`` ）：发送请求并接收响应，尚未打开会话时创建新会话。

    ``TPM_COMM_OP_CLOSE_SESSION`` （ ``0x2`` ）：关闭已有会话，如有。

  ``r5``：in_buffer，请求缓冲区的客户机物理地址。请求与响应可使用同一地址。

  ``r6``：in_size，输入缓冲区大小，不超过 4 KB。

  ``r7``：out_buffer，响应缓冲区的客户机物理地址。请求与响应可使用同一地址。

  ``r8``：out_size，输出缓冲区至少 4 KB，
  因为多数 TPM 实现及 Linux TPM 资源管理器支持的请求／响应最大为 4 KB。

返回值：

  ``r3`` 可为：

    ``H_Success``：请求处理成功。

    ``H_PARAMETER``：无效 TPM 操作。

    ``H_P2``：in_buffer 无效。

    ``H_P3``：in_size 无效。

    ``H_P4``：out_buffer 无效。

    ``H_P5``：out_size 无效。

    ``H_RESOURCE``：与 TPM 通信出错。

    ``H_FUNCTION``：当前未允许或未配置 TPM 访问。

    ``r4``：TPM_COMM_OP_EXECUTE 成功时保存响应大小。
