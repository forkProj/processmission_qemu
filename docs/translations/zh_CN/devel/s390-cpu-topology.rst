S390 CPU 拓扑的 QAPI 接口
=========================

以下结合输出示例介绍 S390 CPU 拓扑的 QAPI 接口。
假设使用下面命令启动 QEMU，共定义 4 个 CPU，
其中 CPU[0] 由 -smp 参数定义，使用默认值：

.. code-block:: bash

 qemu-system-s390x \
    -enable-kvm \
    -cpu z14,ctop=on \
    -smp 1,drawers=3,books=3,sockets=2,cores=2,maxcpus=36 \
    -device z14-s390x-cpu,core-id=19,entitlement=high \
    -device z14-s390x-cpu,core-id=11,entitlement=low \
    -device z14-s390x-cpu,core-id=12,entitlement=high \
   ...

query-cpus-fast 的扩展
----------------------

query-cpus-fast 查询全部已配置 vCPU 的拓扑树和修饰属性。

.. code-block:: QMP

 { "execute": "query-cpus-fast" }
 {
  "return": [
    {
      "dedicated": false,
      "thread-id": 536993,
      "props": {
        "core-id": 0,
        "socket-id": 0,
        "drawer-id": 0,
        "book-id": 0
      },
      "cpu-state": "operating",
      "entitlement": "medium",
      "qom-path": "/machine/unattached/device[0]",
      "cpu-index": 0,
      "target": "s390x"
    },
    {
      "dedicated": false,
      "thread-id": 537003,
      "props": {
        "core-id": 19,
        "socket-id": 1,
        "drawer-id": 0,
        "book-id": 2
      },
      "cpu-state": "operating",
      "entitlement": "high",
      "qom-path": "/machine/peripheral-anon/device[0]",
      "cpu-index": 19,
      "target": "s390x"
    },
    {
      "dedicated": false,
      "thread-id": 537004,
      "props": {
        "core-id": 11,
        "socket-id": 1,
        "drawer-id": 0,
        "book-id": 1
      },
      "cpu-state": "operating",
      "entitlement": "low",
      "qom-path": "/machine/peripheral-anon/device[1]",
      "cpu-index": 11,
      "target": "s390x"
    },
    {
      "dedicated": true,
      "thread-id": 537005,
      "props": {
        "core-id": 12,
        "socket-id": 0,
        "drawer-id": 3,
        "book-id": 2
      },
      "cpu-state": "operating",
      "entitlement": "high",
      "qom-path": "/machine/peripheral-anon/device[2]",
      "cpu-index": 12,
      "target": "s390x"
    }
  ]
 }


QAPI 命令：set-cpu-topology
---------------------------

set-cpu-topology 修改配置中某个 vCPU 的拓扑位置或拓扑修饰属性。

.. code-block:: QMP

    { "execute": "set-cpu-topology",
      "arguments": {
         "core-id": 11,
         "socket-id": 0,
         "book-id": 0,
         "drawer-id": 0,
         "entitlement": "low",
         "dedicated": false
      }
    }
    {"return": {}}

只有 core-id 是必需参数，其他未指定参数保持原值。

QAPI 事件 CPU_POLARIZATION_CHANGE
---------------------------------

客户机请求修改极化方式时，QEMU 发送 CPU_POLARIZATION_CHANGE 事件。

客户机只指定水平或垂直极化。
管理 QEMU 的实体负责响应事件，设置独占属性及更细粒度的垂直资源权益。

垂直极化的独占 vCPU 只能具有高权益，因此共有六种组合：

- 水平
- 水平、独占
- 垂直、低权益
- 垂直、中权益
- 垂直、高权益
- 垂直、高权益、独占

客户机执行 Perform Topology Function 指令 PTF(0) 请求水平极化时，
收到的事件示例：

.. code-block:: QMP

  {
    "timestamp": {
      "seconds": 1687870305,
      "microseconds": 566299
    },
    "event": "CPU_POLARIZATION_CHANGE",
    "data": {
      "polarization": "horizontal"
    }
  }

QAPI 查询命令：query-s390x-cpu-polarization
-------------------------------------------

query-s390x-cpu-polarization 返回机器当前的 CPU 极化方式。
下例中客户机此前已执行 PTF(1) 请求垂直极化：

.. code-block:: QMP

    { "execute": "query-s390x-cpu-polarization" }
    {
        "return": {
          "polarization": "vertical"
        }
    }
