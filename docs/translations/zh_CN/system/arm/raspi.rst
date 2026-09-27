Raspberry Pi 开发板（``raspi0``、``raspi1ap``、``raspi2b``、``raspi3ap``、``raspi3b``、``raspi4b``）
====================================================================================================

QEMU 提供以下模型：

``raspi0`` 和 ``raspi1ap``
  ARM1176JZF-S 内核，512 MiB RAM
``raspi2b``
  4 核 Cortex-A7，1 GiB RAM
``raspi3ap``
  4 核 Cortex-A53，512 MiB RAM
``raspi3b``
  4 核 Cortex-A53，1 GiB RAM
``raspi4b``
  4 核 Cortex-A72，2 GiB RAM

已实现设备
-------------------

 * ARM1176JZF-S、Cortex-A7、Cortex-A53 或 Cortex-A72 CPU
 * 中断控制器
 * DMA 控制器
 * 时钟和复位控制器（CPRMAN）
 * 系统定时器
 * GPIO 控制器
 * 串口（基于 16550 的 BCM2835 AUX 和 PL011）
 * 随机数生成器（RNG）
 * 帧缓冲
 * USB 主机（USBH）
 * GPIO 控制器
 * SD/MMC 主机控制器
 * SoC 温度传感器
 * USB2 主机控制器（DWC2 和 MPHI）
 * 邮箱控制器（MBOX）
 * VideoCore 固件（属性接口）
 * 外设 SPI 控制器
 * Broadcom 串行控制器（I2C）

缺失设备
---------------

 * 脉宽调制（PWM）
 * PCIE 根端口（raspi4b）
 * GENET 以太网控制器（raspi4b）
