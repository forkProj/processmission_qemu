.. _QEMU-PC-System-emulator:

x86 系统模拟器
-------------------

开发板专用文档
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

..
   此目录应按各文件的标题文本保持字母顺序，
   这与按文件名排序的顺序不同。

.. toctree::
   :maxdepth: 1

   i386/pc
   i386/microvm
   i386/nitro-enclave

架构功能
~~~~~~~~~~~~~~~~~~~~~~

.. toctree::
   :maxdepth: 1

   i386/cpu
   i386/hyperv
   i386/xen
   i386/xenpvh
   i386/kvm-pv
   i386/sgx
   i386/amd-memory-encryption
   i386/tdx

操作系统要求
~~~~~~~~~~~~~~~

在 x86_64 主机上，KVM 默认启用的 CPU 功能要求主机运行 Linux 4.5 或更新版本。
