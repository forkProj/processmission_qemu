================================================================
QEMU 与 ACPI BIOS 通用事件设备接口
================================================================

ACPI *通用事件设备* （GED）在 ACPI v6.1 引入，面向硬件精简平台，
处理包括热插拔在内的全部平台事件。GED 在命名空间中建模为设备，
_HID 为 ACPI0013。本文描述 QEMU 与 ACPI BIOS 的接口。

GED 允许硬件精简平台在 ACPI ASL 中处理中断，方式类似 GPIO 事件的 _EVT。
全部中断列在 _CRS，处理程序写在 _EVT。
不过 QEMU 为 GED 仅使用一个中断，通过 I/O 内存区域传递受影响的设备类型，
从而可用一个中断支持最多 32 种事件。

**示例如下：**

::

   Device (\_SB.GED)
   {
       Name (_HID, "ACPI0013")
       Name (_UID, Zero)
       Name (_CRS, ResourceTemplate ()
       {
           Interrupt (ResourceConsumer, Edge, ActiveHigh, Exclusive, ,, )
           {
               0x00000029,
           }
       })
       OperationRegion (EREG, SystemMemory, 0x09080000, 0x04)
       Field (EREG, DWordAcc, NoLock, WriteAsZeros)
       {
           ESEL,   32
       }
       Method (_EVT, 1, Serialized)
       {
           Local0 = ESEL // ESEL = IO memory region which specifies the
                         // device type.
           If (((Local0 & One) == One))
           {
               MethodEvent1()
           }
           If ((Local0 & 0x2) == 0x2)
           {
               MethodEvent2()
           }
           ...
       }
   }

GED I/O 接口（4 字节访问）
------------------------------------------------
**读取：**

::

   [0x0-0x3] QEMU 设置的 32 位事件选择器位域。

    位：
       0: 内存热插拔事件
       1: 系统关机事件
       2: NVDIMM 热插拔事件
       3: CPU 热插拔事件
    4-31: 保留

**写入：**

不应向 GED I/O 内存写入任何内容。
