Virtio 气球内存统计
================================================

virtio balloon 驱动支持客户机内存统计报告。
QEMU 用户可通过轮询机制，以 QOM（QEMU 对象模型）设备属性读取统计。

查询前，客户端必须向 guest-stats-polling-interval 写入秒数以启用轮询：

  > 0
       按指定间隔启用；已启用时，将间隔更新为新值。

  0
       禁用轮询。此前采集的统计仍有效，可以查询。

启用后，QEMU 的 virtio-balloon 按间隔向客户机气球驱动查询新统计。

查询 guest-stats 属性会返回包含以下内容的字典：

  * stats：全部可用统计。客户机不支持或未能获取的项为 -1。
    当前支持：

      - stat-swap-in
      - stat-swap-out
      - stat-major-faults
      - stat-minor-faults
      - stat-free-memory
      - stat-total-memory
      - stat-available-memory
      - stat-disk-caches
      - stat-htlb-pgalloc
      - stat-htlb-pgfail

  * last-update：上次更新的秒级时间戳，由主机生成，故障客户机不能影响其值。
    客户机尚未更新时为 0。

另需注意：

 - 后续禁用轮询后，已获取统计仍可用。

 - 不支持的统计总为 -1，但客户机也可能暂时无法更新某项甚至全部统计。
   此时等待下一次更新即可。

 - 即使客户机不支持统计或未加载驱动，也可启用轮询。此时查询 last-update 为 0。

 - 仅当客户机回复统计请求时，才重新启动轮询计时器。
   故障客户机若始终不回复，计时器不会重启，效果相当于禁用轮询。

以下示例以 ``-device virtio-balloon`` 启动 QEMU，
气球设备的 QOM 路径为 ``/machine/peripheral-anon/device[1]``。

按 2 秒间隔启用轮询::

  { "execute": "qom-set",
               "arguments": { "path": "/machine/peripheral-anon/device[1]",
               "property": "guest-stats-polling-interval", "value": 2 } }

  { "return": {} }

将间隔改为 10 秒::

  { "execute": "qom-set",
               "arguments": { "path": "/machine/peripheral-anon/device[1]",
               "property": "guest-stats-polling-interval", "value": 10 } }

  { "return": {} }

获取统计::

  { "execute": "qom-get",
               "arguments": { "path": "/machine/peripheral-anon/device[1]",
               "property": "guest-stats" } }
  {
    "return": {
        "stats": {
            "stat-swap-out": 0,
            "stat-free-memory": 844943360,
            "stat-minor-faults": 219028,
            "stat-major-faults": 235,
            "stat-total-memory": 1044406272,
            "stat-swap-in": 0
        },
        "last-update": 1358529861
    }
  }

禁用轮询::

  { "execute": "qom-set",
               "arguments": { "path": "/machine/peripheral-anon/device[1]",
               "property": "stats-polling-interval", "value": 0 } }

  { "return": {} }
