Nordic nRF 开发板 (``microbit``)
================================

`Nordic nRF`_ 是基于 Arm 的 SoC 系列，用于低功耗短距离无线方案。

.. _Nordic nRF: https://www.nordicsemi.com/Products

nRF51 是该系列最早用于短距离无线应用的产品，后由 nRF52 取代。
以下机器基于该芯片：

- ``microbit``       采用 nRF51822 SoC 的 BBC micro:bit 开发板

nRF52、nRF53、nRF91 等其他系列目前尚不支持。

支持的设备
-----------------

 * Arm Cortex-M0 (ARMv6-M)
 * 串口（UART）
 * 时钟控制器
 * 定时器
 * 随机数生成器（RNG）
 * GPIO 控制器
 * NVMC
 * SWI

缺失设备
---------------

 * 看门狗
 * 实时时钟（RTC）控制器
 * TWI (i2c)
 * SPI 控制器
 * 模数转换器（ADC）
 * 正交解码器
 * 无线电

启动选项
------------

Micro:bit 可通过 -device 加载 `ihex format`_ 固件来启动，例如：

.. _ihex format: https://en.wikipedia.org/wiki/Intel_HEX

.. code-block:: bash

  $ qemu-system-arm -M microbit -device loader,file=test.hex
