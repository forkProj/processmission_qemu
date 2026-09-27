s390x 上的 PCI 设备
====================

s390x 上的 PCI 设备 work differently than on other architectures and need to
be configured in a slightly different way.

每个 PCI 设备关联一个额外 zpci 设备。未指定时会自动生成，但建议显式
指定，以传入 s390 专用 PCI 配置。

例如，将 0000:00:00.0 直通给客户机::

 qemu-system-s390x ... \
                   -device zpci,uid=1,fid=0,target=hostdev0,id=zpci1 \
                   -device vfio-pci,host=0000:00:00.0,id=hostdev0

zpci 通过 target 属性关联 PCI 设备。

这里不像其他 PCI 实现那样设置客户机总线、插槽和功能号。s390x 不提供
拓扑信息，客户机看不到命令行指定的这些信息。

设备在客户机中的呈现方式由 uid 和 fid 决定。

Linux 用 uid 作为 PCI 标识符的 domain，fid 作为物理插槽，例如::

 qemu-system-s390x ... \
                   -device zpci,uid=7,fid=8,target=hostdev0,id=zpci1 \
                   ...

客户机中显示为::

 # lspci -v
 0007:00:00.0 ...
 Physical Slot: 00000008
 ...
