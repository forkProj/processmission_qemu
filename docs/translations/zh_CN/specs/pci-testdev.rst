========================================
QEMU PCI 测试设备
========================================

``pci-testdev`` 用于测试底层 I/O。

设备最多实现三个 BAR：BAR0、BAR1 和 BAR2。
BAR0 和 BAR1 可以是内存或 I/O；客户机必须检测类型并相应处理。

BAR0 和 BAR1 各自最大 4K 字节，起始处为以下头部：

.. code-block:: c

  typedef struct PCITestDevHdr {
      uint8_t test;        /* write-only, starts a given test number */
      uint8_t width_type;  /*
                            * read-only, type and width of access for a given test.
                            * 1,2,4 for byte,word or long write.
                            * any other value if test not supported on this BAR
                            */
      uint8_t pad0[2];
      uint32_t offset;     /* read-only, offset in this BAR for a given test */
      uint32_t data;       /* read-only, data to use for a given test */
      uint32_t count;      /* for debugging. number of writes detected. */
      uint8_t name[];      /* for debugging. 0-terminated ASCII string. */
  } PCITestDevHdr;

所有寄存器均为小端。

设备应始终在每个 BAR 上实现编号 0 到 N 的测试，并将新增测试赋予更高编号。
这样客户机便可依次扫描测试，直到检测到该 BAR 不支持的访问类型后停止。

BAR2 为没有后备存储的 64 位内存 BAR，默认禁用，可通过 ``membar=<size>`` 启用。
它可用于测试客户机能否正确处理特定大小、甚至很大的 PCI BAR。
