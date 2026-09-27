Arm Musca 开发板（``musca-a``、``musca-b1``）
=============================================

Arm Musca 是使用 SSE-200 嵌入式子系统的参考开发板，具有两个 Cortex-M33。
QEMU 提供 A 和 B1 两种变体。

未实现设备：

- SPI
- |I2C|
- |I2S|
- PWM
- QSPI
- 定时器
- SCC
- GPIO
- eFlash
- MHU
- PVT
- SDIO
- CryptoCell

与真实硬件一样，Musca-A 不对称：CPU 0 没有 FPU 和 DSP 扩展，CPU 1 则有。
A 与 B1 的内存映射也差异很大，因此客户机软件必须针对正确变体构建。
