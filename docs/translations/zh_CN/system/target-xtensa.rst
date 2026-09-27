.. _Xtensa-System-emulator:

Xtensa 系统模拟器
----------------------

``qemu-system-xtensa`` 和 ``qemu-system-xtensaeb`` 分别覆盖 Xtensa 的两种
字节序，模拟两类机器：

-  Xtensa 模拟器伪开发板 \"sim\"

-  Avnet LX60/LX110/LX200 开发板

sim 提供类似专有 Tensilica ISS 的环境，支持：

-  多种 Xtensa CPU，默认为 DC232B

-  通过半主机调用访问控制台和文件系统

Avnet LX60/LX110/LX200 模拟支持：

-  多种 Xtensa CPU，默认为 DC232B

-  16550 UART

-  OpenCores 10/100 Mbps 以太网 MAC
