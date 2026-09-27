STMicroelectronics STM32 板（``netduino2``、``netduinoplus2``、``olimex-stm32-h405``、``stm32vldiscovery``）
===============================================================================================================

`STM32`_ 芯片是 STMicroelectronics 推出的基于 32 位 Arm 的微控制器系列。

.. _STM32: https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html

STM32F1 系列基于 Arm Cortex-M3 核心。以下机器基于此芯片：

- ``stm32vldiscovery``  采用 STM32F100RBT6 微控制器的 STM32VLDISCOVERY 板

STM32F2 系列基于 Arm Cortex-M3 核心。以下机器基于此芯片：

- ``netduino2``         采用 STM32F205RFT6 微控制器的 Netduino 2 板

STM32F4 系列和 STM32L4 超低功耗系列均基于 Arm Cortex-M4F 核心。STM32F4 系列与 STM32F2 系列引脚兼容。以下机器基于此 Arm Cortex-M4F 芯片：

- ``netduinoplus2``     采用 STM32F405RGT6 微控制器的 Netduino Plus 2 板
- ``olimex-stm32-h405`` 采用 STM32F405RGT6 微控制器的 Olimex STM32 H405 板
- ``b-l475e-iot01a``    采用 STM32L475VG 微控制器的 :doc:`B-L475E-IOT01A IoT Node </system/arm/b-l475e-iot01a>` 板

还有许多其他 STM32 系列目前不受 QEMU 支持。

支持的设备
-----------------

 * Arm Cortex-M3、Cortex-M4F
 * 模数转换器（ADC）
 * EXTI 中断
 * 串口（USART）
 * SPI 控制器
 * 系统配置（SYSCFG）
 * 定时器控制器（TIMER）
 * 复位和时钟控制器（RCC）（仅 STM32F4，仅支持复位和使能）

缺少的设备
---------------

 * 摄像头接口（DCMI）
 * 控制器局域网（CAN）
 * 循环冗余校验（CRC）计算单元
 * 数模转换器（DAC）
 * DMA 控制器
 * 以太网控制器
 * 闪存接口单元
 * GPIO 控制器
 * I2C 控制器
 * 集成电路内置音频（I2S）控制器
 * 电源配置（PWR）
 * 随机数生成器（RNG）
 * 实时时钟（RTC）控制器
 * 复位和时钟控制器（RCC）（复位和使能之外的功能）
 * 安全数字输入/输出（SDIO）接口
 * USB OTG
 * 看门狗控制器（IWDG、WWDG）

启动选项
------------

STM32 机器可以通过 ``-kernel`` 选项加载固件并启动。例如：

.. code-block:: bash

  $ qemu-system-arm -M stm32vldiscovery -kernel firmware.bin
