B-L475E-IOT01A 物联网节点 (``b-l475e-iot01a``)
==============================================

B-L475E-IOT01A 物联网节点采用基于 Arm Cortex-M4F 核心的 STM32L475VG SoC，
属于 STMicroelectronics 的 :doc:`STM32 板系列 </system/arm/stm32>`，
具体为 STM32L4 超低功耗系列。STM32L4x5 芯片最高运行频率为 80 MHz，
集成 128 KiB SRAM 和最多 1 MiB 闪存。B-L475E-IOT01A 板提供
64 Mibit QSPI 闪存、蓝牙、WiFi 和射频连接，以及 USART、I2C、SPI、CAN、
USB OTG 和多种传感器。

支持的设备
"""""""""""""""""

目前支持：

- 基于 Cortex-M4F 的 STM32L4x5 SoC
- STM32L4x5 EXTI（扩展中断和事件控制器）
- STM32L4x5 SYSCFG（系统配置控制器）
- STM32L4x5 RCC（复位和时钟控制）
- STM32L4x5 GPIO（通用 I/O）
- STM32L4x5 USART、UART 和 LPUART（串口）
- 可选 8x8 LED 显示器（基于 DM163 驱动）

缺失设备
"""""""""""""""

以下设备尚不支持：

- 模数转换器（ADC）
- SPI 控制器
- 定时器控制器（TIMER）

完整未实现外设列表见 ``./hw/arm/stm32l4x5_soc.c``。

启动选项
""""""""""""

用 -kernel 加载固件启动，例如：

.. code-block:: bash

  $ qemu-system-arm -M b-l475e-iot01a -kernel firmware.bin
