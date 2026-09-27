===================
虚拟 CPU 热插拔
===================

以下是使用 QMP device_add 和 device_del 热插入、热移除 vCPU 的完整示例。

vCPU 热插入
------------

(1) 启动 QEMU，maxcpus 必须指定，才能热插入::

      $ qemu-system-x86_64 -display none -no-user-config -m 2048 \
          -nodefaults -monitor stdio -machine pc,accel=kvm,usb=off \
          -smp 1,maxcpus=2 -cpu IvyBridge-IBRS \
          -qmp unix:/tmp/qmp-sock,server=on,wait=off


(2) 用源码树 scripts/qmp/ 中的 qmp-shell 连接刚启动的 QEMU::

      $> ./qmp-shell -p -v /tmp/qmp-sock
      [...]
      (QEMU)


(3) 查询哪些 CPU 类型可以插入，以及可用插槽::

      (QEMU) query-hotpluggable-cpus
      {
          "execute": "query-hotpluggable-cpus",
          "arguments": {}
      }
      {
          "return": [
              {
                  "props": {
                      "core-id": 1,
                      "socket-id": 0,
                      "thread-id": 0
                  },
                  "type": "IvyBridge-IBRS-x86_64-cpu",
                  "vcpus-count": 1
              },
              {
                  "props": {
                      "core-id": 0,
                      "socket-id": 0,
                      "thread-id": 0
                  },
                  "qom-path": "/machine/unattached/device[0]",
                  "type": "IvyBridge-IBRS-x86_64-cpu",
                  "vcpus-count": 1
              }
          ]
      }
      (QEMU)


(4) 返回对象中，有 qom-path 的 CPU 已存在，没有的可热插入。上例显示
    插槽 0、核心 0 已有 IvyBridge-IBRS-x86_64-cpu。要在插槽 0、核心 1
    插入，需要将列出的属性传给 device_add::

      (QEMU) device_add id=cpu-2 driver=IvyBridge-IBRS-x86_64-cpu socket-id=0 core-id=1 thread-id=0
      {
          "execute": "device_add",
          "arguments": {
              "core-id": 1,
              "driver": "IvyBridge-IBRS-x86_64-cpu",
              "id": "cpu-2",
              "socket-id": 0,
              "thread-id": 0
          }
      }
      {
          "return": {}
      }
      (QEMU)


(5) 可选运行 query-cpus-fast 查看详情::

      (QEMU) query-cpus-fast
      {
          "arguments": {}
          "execute": "query-cpus-fast",
      }
      {
          "return": [
              {
                  "cpu-index": 0,
                  "props": {
                      "core-id": 0,
                      "socket-id": 0,
                      "thread-id": 0
                  },
                  "qom-path": "/machine/unattached/device[0]",
                  "target": "x86_64",
                  "thread-id": 28957
              },
              {
                  "cpu-index": 1,
                  "props": {
                      "core-id": 1,
                      "socket-id": 0,
                      "thread-id": 0
                  },
                  "qom-path": "/machine/peripheral/cpu-2",
                  "target": "x86_64",
                  "thread-id": 29095
              }
          ]
      }
      (QEMU)


vCPU 热移除
---------------

从 qmp-shell 调用 device_del::

      (QEMU) device_del id=cpu-2
      {
          "arguments": {
              "id": "cpu-2"
          }
          "execute": "device_del",
      }
      {
          "return": {}
      }
      (QEMU)


.. note::
    热移除需要客户机配合，因此此命令只是请求，不保证立即移除。客户机收到
    系统控制中断（SCI），调用相关 vCPU 的 ACPI 处理程序，再由内核将 CPU
    下线并通知 QEMU 移除。
