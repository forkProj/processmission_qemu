虚拟机间共享内存设备
-----------------------------

Linux 主机支持共享内存设备，基本语法为：

.. parsed-literal::

   |qemu_system_x86| -device ivshmem-plain,memdev=hostmem


hostmem 是主机内存后端名称。POSIX 共享内存后端可配置为：

::

   -object memory-backend-file,size=1M,share,mem-path=/dev/shm/ivshmem,id=hostmem


访问同一共享区域的虚拟机之间还可发送中断，需要共享内存服务器及连接它的
chardev 套接字。服务器源码在 qemu.git/contrib/ivshmem-server。示例：

.. parsed-literal::

   # First start the ivshmem server once and for all
   ivshmem-server -p pidfile -S path -m shm-name -l shm-size -n vectors

   # Then start your qemu instances with matching arguments
   |qemu_system_x86| -device ivshmem-doorbell,vectors=vectors,chardev=id
                    -chardev socket,path=path,id=id


服务器为客户机分配非负 VM ID，同一服务器上的客户机可通过中断通信。
ID 可从设备寄存器读取，见 :doc:`../../specs/ivshmem-spec`。

ivshmem 迁移
~~~~~~~~~~~~~~~~~~~~~~

master=on 时，迁移会把共享内存复制到目标主机；master=off 时，设备仍连接
则无法迁移，需要先热移除，再在迁移后通过 PCI 热插拔重新连接。

共享同一内存的设备最多一个为 master，必须先完成其迁移，再插回其他设备。

ivshmem 与大页
~~~~~~~~~~~~~~~~~~~~~

除 POSIX shm 指定大小外，也可使用支持大页的后端：

.. parsed-literal::

   |qemu_system_x86| -object memory-backend-file,size=1G,mem-path=/dev/hugepages/my-shmem-file,share,id=mb1
                    -device ivshmem-plain,memdev=mb1


ivshmem-server 的 -m 内存路径参数也支持大页挂载点。
