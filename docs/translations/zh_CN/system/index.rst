.. _System Emulation:

----------------
系统模拟
----------------

本节是使用 QEMU 进行完整系统模拟的总体指南，与用户模式模拟相区别。
内容也包括与 KVM、Xen、Hypervisor.Framework 等虚拟机监控程序配合使用。

.. toctree::
   :maxdepth: 3

   introduction
   invocation
   device-emulation
   keys
   mux-chardev
   monitor
   images
   virtio-net-failover
   linuxboot
   generic-loader
   guest-loader
   barrier
   vnc-security
   tls
   secrets
   authz
   gdb
   replay
   managed-startup
   bootindex
   cpu-hotplug
   pr-manager
   targets
   security
   multi-process
   confidential-guest-support
   igvm
   nitro
   whpx
   vm-templating
   sriov
   qemu-colo
