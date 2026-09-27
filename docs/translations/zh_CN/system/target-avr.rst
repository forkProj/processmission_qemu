.. _AVR-System-emulator:

AVR 系统模拟器
-------------------

使用 ``qemu-system-avr`` 模拟基于 AVR 8 位处理器的机器，可选内核包括：
avr1、avr2、avr25、avr3、avr31、avr35、avr4、avr5、avr51、avr6、avrtiny、
xmega2、xmega3、xmega4、xmega5、xmega6 和 xmega7。

目前支持少量用于教学和测试的 Arduino 开发板。这些开发板使用 ATmega 控制器，
模型只实现 USART 和 16 位定时器，足以运行基于 FreeRTOS 的应用，例如：
https://github.com/seharris/qemu-avr-tests/blob/master/free-rtos/Demo/AVR_ATMega2560_GCC/demo.elf

假定 demo.elf 已针对 AVR CPU 编译，用法示例如下：

- 持续执行且不暂停::

   qemu-system-avr -machine mega2560 -bios demo.elf

- 持续执行，并将串口输出发送到 telnet 窗口::

   qemu-system-avr -M mega2560 -bios demo.elf -nographic \
                   -serial tcp::5678,server=on,wait=off

  然后在另一个 Shell 中运行::

   telnet localhost 5678

- 使用 GDB 调试::

   qemu-system-avr -machine mega2560 -bios demo.elf -s -S

  然后在另一个 Shell 中运行::

   avr-gdb demo.elf

  接着在 GDB 中执行::

   target remote :1234

- 输出执行的指令，仅限尚未被 JIT 编译器翻译的指令::

   qemu-system-avr -machine mega2560 -bios demo.elf -d in_asm
