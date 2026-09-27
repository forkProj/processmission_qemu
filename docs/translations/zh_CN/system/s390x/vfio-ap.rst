辅助处理器（AP）设备
=============================

.. contents::

简介
------------

IBM 辅助处理器（AP）密码设施由三条 AP 指令和 1 至 256 张 PCIe 密码适配器卡组成。这些 AP 设备为分配给 Linux 系统的所有 CPU 提供密码功能，该 Linux 系统运行在 IBM Z 系统的 LPAR 中。

在 s390x 上，AP 适配器卡通过 AP 总线提供。本文说明如何使用 VFIO 中介设备框架，将这些卡提供给 KVM 客户机。

AP 架构概述
-------------------------

为理解本文后续使用的术语，先给出一些定义：

* AP 适配器

  AP 适配器是能够执行密码功能的 IBM Z 适配器卡。根据机器型号，一个 LPAR 可以分配 0 至 256 个适配器。分配给 Linux 主机所在 LPAR 的适配器，可由该 Linux 主机使用。每个适配器以 0 至 255 的编号标识，但允许的最大适配器编号由机器型号决定。安装后，任何 CPU 均可通过执行 AP 指令访问 AP 适配器。

* AP 域

  适配器被划分为多个域。每个域可视为一组用于处理 AP 指令的硬件寄存器。一个适配器最多容纳 256 个域，但允许的最大域编号由机器型号决定。每个域以 0 至 255 的编号标识。域还可进一步分为两类：

    * 使用域：可直接访问以处理 AP 命令的域。

    * 控制域：通过发送给使用域的 AP 命令间接访问，以控制或更改该域；例如，为该域设置安全私钥。

* AP 队列

  AP 队列用于将 AP 命令请求消息发送到特定 AP 内的某个 AP 使用域。AP 队列由 AP 适配器 ID（APID）和 AP 队列索引（APQI）组成的二元组标识。APQI 对应适配器内的某个使用域编号。此二元组构成 AP 队列号（APQN），唯一标识一个 AP 队列。AP 指令包含一个存放 APQN 的字段，用来标识应接收并处理 AP 命令请求消息的 AP 队列。

* AP 指令：

  AP 指令有三条：

  * NQAP：将 AP 命令请求消息加入队列
  * DQAP：从队列中取出 AP 命令回复消息
  * PQAP：管理队列

  AP 指令标识处理 AP 命令的目标域；该域必须是使用域之一。AP 命令可以修改不属于使用域的域，但被修改的域必须是控制域之一。

启动解释执行（SIE）指令
----------------------------------------------

KVM 客户机通过执行启动解释执行（SIE）指令启动。SIE 状态描述是包含 KVM 客户机状态信息的控制块，作为输入提供给 SIE 指令。SIE 状态描述包含一个附属控制块，称为密码控制块（CRYCB）。CRYCB 包含三个字段，用于标识分配给 KVM 客户机的适配器、使用域和控制域：

* AP 掩码（APM）字段是一个位掩码，用于标识分配给 KVM 客户机的 AP 适配器。掩码中的每一位从左至右对应 0 至 255 的 APID。如果某位置位，对应适配器就可供 KVM 客户机使用。

* AP 队列掩码（AQM）字段是一个位掩码，用于标识分配给 KVM 客户机的 AP 使用域。掩码中的每一位从左至右对应 0 至 255 的 AP 队列索引（APQI）。如果某位置位，对应队列就可供 KVM 客户机使用。

* AP 域掩码字段是一个位掩码，用于标识分配给 KVM 客户机的 AP 控制域。ADM 位掩码控制客户机向使用域发送的 AP 命令请求消息可以修改哪些域。掩码中的每一位从左至右对应 0 至 255 的域。如果某位置位，对应域就可以被发送到使用域的 AP 命令请求消息修改。

如 AP 队列的定义所述，AP 指令包含 APQN，用来标识发送 AP 命令请求消息（NQAP 和 PQAP 指令）或接收命令回复消息（DQAP 指令）的 AP 适配器及 AP 队列。APQN 是否有效由 APM 和 AQM 计算出的矩阵定义：它是所有已分配适配器编号（APM）与所有已分配队列索引（AQM）的笛卡尔积。例如，如果向客户机分配适配器 1、2 和使用域 5、6，则 APQN (1,5)、(1,6)、(2,5) 和 (2,6) 对该客户机有效。

APQN 可以提供安全密钥功能，即在适配器卡上为其每个域存储私钥，因此每个 APQN 最多只能分配给一个客户机或 Linux 主机。

示例 1：有效配置
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+--------+--------+
|          | 客户机1| 客户机2|
+==========+========+========+
| 适配器   |  1, 2  |  1, 2  |
+----------+--------+--------+
| 域       |  5, 6  |  7     |
+----------+--------+--------+

此配置有效，因为两个客户机各自具有互不重叠的 APQN 集合：

* 客户机 1 具有 APQN (1,5)、(1,6)、(2,5) 和 (2,6)；
* 客户机 2 具有 APQN (1,7) 和 (2,7)。

示例 2：有效配置
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+--------+--------+
|          | 客户机1| 客户机2|
+==========+========+========+
| 适配器   |  1, 2  |  3, 4  |
+----------+--------+--------+
| 域       |  5, 6  |  5, 6  |
+----------+--------+--------+

此配置同样有效，因为两个客户机各自具有互不重叠的 APQN 集合：

* 客户机 1 具有 APQN (1,5)、(1,6)、(2,5)、(2,6)；
* 客户机 2 具有 APQN (3,5)、(3,6)、(4,5)、(4,6)。

示例 3：无效配置
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+--------+--------+
|          | 客户机1| 客户机2|
+==========+========+========+
| 适配器   |  1, 2  |  1     |
+----------+--------+--------+
| 域       |  5, 6  |  6, 7  |
+----------+--------+--------+

此配置无效，因为两个客户机都能够访问 APQN (1,6)。

Linux 主机上的 AP 矩阵配置
-------------------------------------

Linux 系统是其所在 LPAR 的客户机，可以访问为该 LPAR 配置的 AP 资源。LPAR 的 AP 矩阵通过其激活配置文件配置，该文件可以在 HMC 上编辑。Linux 系统启动时，AP 总线检测分配给 LPAR 的 AP 设备，并在 sysfs 中创建以下内容::

  /sys/bus/ap
  ... [devices]
  ...... xx.yyyy
  ...... ...
  ...... cardxx
  ...... ...


其中：

``cardxx``
  表示编号为 xx（十六进制）的 AP 适配器。

``xx.yyyy``
  表示 APQN，xx 指定 APID，yyyy 指定 APQI。

例如，如果为 LPAR 配置了 AP 适配器 5、6 和域 4、71（0x47）、171（0xab）、255（0xff），则 Linux 主机系统上的 sysfs 表示如下::

  /sys/bus/ap
  ... [devices]
  ...... 05.0004
  ...... 05.0047
  ...... 05.00ab
  ...... 05.00ff
  ...... 06.0004
  ...... 06.0047
  ...... 06.00ab
  ...... 06.00ff
  ...... card05
  ...... card06


还会创建一组默认设备驱动，控制可分配给 Linux 主机所在 LPAR 的各类 AP 设备::

  /sys/bus/ap
  ... [drivers]
  ...... [cex2acard]        for Crypto Express 2/3 accelerator cards
  ...... [cex2aqueue]       for AP queues served by Crypto Express 2/3
                            accelerator cards
  ...... [cex4card]         for Crypto Express 4/5/6 accelerator and coprocessor
                            cards
  ...... [cex4queue]        for AP queues served by Crypto Express 4/5/6
                            accelerator and coprocessor cards
  ...... [pcixcccard]       for Crypto Express 2/3 coprocessor cards
  ...... [pcixccqueue]      for AP queues served by Crypto Express 2/3
                            coprocessor cards


将 AP 设备绑定到设备驱动
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

有两个 sysfs 文件指定了位掩码，将 APQN 范围的子集标记为“可由默认 AP 队列设备驱动使用”或“不可由默认设备驱动使用”；后者可供替代设备驱动使用。掩码在 sysfs 中的位置为::

   /sys/bus/ap/apmask
   /sys/bus/ap/aqmask


``apmask`` 是一个 256 位掩码，标识一组 AP 适配器 ID（APID）。掩码中每一位从左至右（即按大端顺序从最高有效位到最低有效位）对应 0 至 255 的 APID。如果某位置位，相应 APID 被标记为仅可由默认 AP 队列设备驱动使用；否则，该 APID 可由 vfio_ap 设备驱动使用。

``aqmask`` 是一个 256 位掩码，标识一组 AP 队列索引（APQI）。掩码中每一位从左至右（即按大端顺序从最高有效位到最低有效位）对应 0 至 255 的 APQI。如果某位置位，相应 APQI 被标记为仅可由默认 AP 队列设备驱动使用；否则，该 APQI 可由 vfio_ap 设备驱动使用。

例如，考虑以下掩码::

      0x7dffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff


它表示：

      1、2、3、4、5 和 7 至 255 属于默认驱动池，0 和 6 属于 vfio_ap 设备驱动池。

AP 总线会检查分配给 Linux 主机的每个 AP 队列设备的 APQN，将其与标记为仅供默认 AP 队列设备驱动使用的 APID 和 APQI 的笛卡尔积所构成的 APQN 集合比较。如果匹配，则仅探测默认 AP 队列设备驱动；否则，探测 vfio_ap 设备驱动。

默认情况下，这两个掩码将所有 APQN 保留给默认 AP 队列设备驱动使用。可以通过两种方式修改默认掩码：

 1. 将字符串 echo 到相应 sysfs 掩码文件中，以编辑该文件。支持两种格式：

    * 以 0x 开头的绝对十六进制字符串（例如 "0x12345678"）设置掩码。如果字符串短于掩码，则在右侧补 0；例如，掩码值 0x41 等同于::

           0x4100000000000000000000000000000000000000000000000000000000000000


      请记住，掩码从左至右读取（即按大端顺序从最高有效位到最低有效位），因此上述掩码标识设备号 1 和 7（``01000001``）。

      如果字符串长于掩码，操作将终止并返回错误 EINVAL。

    * 可以用逗号分隔的列表指定要切换的位号，以单独设置或清除掩码中的位。每个位号字符串必须以加号（``+``）或减号（``-``）为前缀，表示将对应位置位（``+``）或清零（``-``）。一些有效值如下::

           "+0"    switches bit 0 on
           "-13"   switches bit 13 off
           "+0x41" switches bit 65 on
           "-0xff" switches bit 255 off


      以下示例::

              +0,-6,+0x47,-0xf0


      将位 0 和 71（0x47）置位，
      将位 6 和 240（0xf0）清零。

      注意，列表中未指定的位保持操作前的状态。

 2. 也可以在启动时通过内核命令行参数修改掩码，例如::

         ap.apmask=0xffff ap.aqmask=0x40


    这会创建以下掩码：

    apmask::

            0xffff000000000000000000000000000000000000000000000000000000000000


    aqmask::

            0x4000000000000000000000000000000000000000000000000000000000000000


    由此形成以下两个池::

            default drivers pool:    adapter 0-15, domain 1
            alternate drivers pool:  adapter 16-255, domains 0, 2-255


为 Linux 客户机配置 AP 矩阵
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

用于为客户机配置 AP 矩阵的 sysfs 接口建立在 VFIO 中介设备框架之上。要为客户机配置 AP 矩阵，首先必须为 ``/sys/devices/vfio_ap/matrix`` 设备创建一个中介矩阵设备。vfio_ap 设备驱动加载时会注册到 VFIO 中介设备框架，并在注册时创建用于创建中介矩阵设备的 sysfs 接口::

  /sys/devices
  ... [vfio_ap]
  ......[matrix]
  ......... [mdev_supported_types]
  ............ [vfio_ap-passthrough]
  ............... create
  ............... [devices]


向名为 ``create`` 的属性文件写入 UUID，即可创建中介 AP 矩阵设备，例如::

   uuidgen > create


或者

::

   echo $uuid > create


创建中介 AP 矩阵设备时，会在 ``devices`` 子目录中创建以 UUID 命名的 sysfs 目录::

  /sys/devices
  ... [vfio_ap]
  ......[matrix]
  ......... [mdev_supported_types]
  ............ [vfio_ap-passthrough]
  ............... create
  ............... [devices]
  .................. [$uuid]


还会在中介矩阵设备的 sysfs 目录中创建三组属性文件，用于配置 KVM 客户机的 AP 矩阵::

  /sys/devices
  ... [vfio_ap]
  ......[matrix]
  ......... [mdev_supported_types]
  ............ [vfio_ap-passthrough]
  ............... create
  ............... [devices]
  .................. [$uuid]
  ..................... assign_adapter
  ..................... assign_control_domain
  ..................... assign_domain
  ..................... matrix
  ..................... unassign_adapter
  ..................... unassign_control_domain
  ..................... unassign_domain


``assign_adapter``
   要将 AP 适配器分配给中介矩阵设备，将其 APID 写入 ``assign_adapter`` 文件。可以多次执行以分配多个适配器。APID 可以按常规语法指定为十进制、十六进制或八进制数。例如，分别以十进制、十六进制和八进制向中介矩阵设备分配适配器 4、5 和 16::

       echo 4 > assign_adapter
       echo 0x5 > assign_adapter
       echo 020 > assign_adapter


   要成功分配适配器，必须满足：

   * 指定的适配器编号必须介于 0 与机器型号允许的最大适配器编号之间。如果超过最大值，操作将终止并返回 ENODEV 错误。

   * 由待分配适配器 ID 和此前已分配域 ID 推导出的所有 APQN，都必须绑定到 vfio_ap 设备驱动。如果尚未分配任何域，则至少必须有一个具有指定 APID 的 APQN 绑定到 vfio_ap 驱动。如果没有这样的 APQN 绑定到驱动，操作将终止并返回 EADDRNOTAVAIL 错误。

   * 由该适配器 ID 和此前已分配域 ID 推导出的任何 APQN，都不能分配给其他中介矩阵设备。如果某个 APQN 已分配给其他中介矩阵设备，操作将终止并返回 EADDRINUSE 错误。

``unassign_adapter``
   要取消分配 AP 适配器，将其 APID 写入 ``unassign_adapter`` 文件。也可以多次执行以取消分配多个适配器。

``assign_domain``
   要分配使用域，将域号写入 ``assign_domain`` 文件。可以多次执行以分配多个使用域。域号按常规语法指定为十进制、十六进制或八进制数。例如，分别以十进制、十六进制和八进制向中介矩阵设备分配使用域 4、8 和 71::

      echo 4 > assign_domain
      echo 0x8 > assign_domain
      echo 0107 > assign_domain


   要成功分配域，必须满足：

   * 指定的域号必须介于 0 与机器型号允许的最大域号之间。如果超过最大值，操作将终止并返回 ENODEV 错误。

   * 由待分配域 ID 和此前已分配适配器 ID 推导出的所有 APQN，都必须绑定到 vfio_ap 设备驱动。如果尚未分配任何域，则至少必须有一个具有指定 APQI 的 APQN 绑定到 vfio_ap 驱动。如果没有这样的 APQN 绑定到驱动，操作将终止并返回 EADDRNOTAVAIL 错误。

   * 由待分配域 ID 和此前已分配适配器 ID 推导出的任何 APQN，都不能分配给其他中介矩阵设备。如果某个 APQN 已分配给其他中介矩阵设备，操作将终止并返回 EADDRINUSE 错误。

``unassign_domain``
   要取消分配使用域，将域号写入 ``unassign_domain`` 文件。可以多次执行以取消分配多个使用域。

``assign_control_domain``
   要分配控制域，将域号写入 ``assign_control_domain`` 文件。可以多次执行以分配多个控制域。域号可按常规语法指定为十进制、十六进制或八进制数。例如，分别以十进制、十六进制和八进制向中介矩阵设备分配控制域 4、8 和 71::

      echo 4 > assign_domain
      echo 0x8 > assign_domain
      echo 0107 > assign_domain


   要成功分配控制域，指定的域号必须介于 0 与机器型号允许的最大域号之间。如果超过最大值，操作将终止并返回 ENODEV 错误。

``unassign_control_domain``
   要取消分配控制域，将域号写入 ``unassign_domain`` 文件。可以多次执行以取消分配多个控制域。

注意：客户机正在使用中介矩阵设备运行时，不允许更改 AP 矩阵。分配适配器、域或控制域的尝试会被拒绝，并返回 EBUSY 错误。

启动配置了 AP 矩阵的 Linux 客户机
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

要提供中介矩阵设备供客户机使用，必须在 QEMU 命令行中指定以下选项::

   -device vfio_ap,sysfsdev=$path-to-mdev


sysfsdev 参数指定中介矩阵设备的路径。可以通过多种方式指定该路径::

  /sys/devices/vfio_ap/matrix/$uuid
  /sys/bus/mdev/devices/$uuid
  /sys/bus/mdev/drivers/vfio_mdev/$uuid
  /sys/devices/vfio_ap/matrix/mdev_supported_types/vfio_ap-passthrough/devices/$uuid


Linux 客户机启动时，会打开中介矩阵设备的文件描述符，获取该设备的信息。``vfio_ap`` 设备驱动使用通过中介矩阵设备 sysfs 属性文件分配的适配器、使用域和控制域，更新客户机 CRYCB 中的 APM、AQM 和 ADM 字段。随后，在 Linux 客户机上运行的程序将能够：

1. 直接访问由客户机 CRYCB 的 APM 和 AQM 字段分别指定的 AP 适配器编号（APID）与队列索引（APQI）的笛卡尔积推导出的 APQN。这些 APQN 标识客户机可用的 AP 队列，也就是说，客户机可以向其中任意队列发送 AP 命令进行处理。

2. 获得授权，通过处理 AP 命令修改客户机 CRYCB 的 ADM 字段标识的控制域。AP 命令必须发送到有效的 APQN（参见上述第 1 点）。

CPU 模型功能：

有三个 CPU 模型功能可用于控制客户机对 AP 设施的访问：

1. AP 设施功能

   AP 设施功能指示客户机安装了 AP 设施。只有主机系统安装了 AP 设施时，才会提供此功能。该功能为 s390 专用，在 QEMU 命令行中表示为 -cpu 选项的参数::

      qemu-system-s390x -cpu $model,ap=on|off


   其中：

      ``$model``
        是为客户机定义的 CPU 模型（未指定时默认为主机系统的模型）。

      ``ap=on|off``
        指示是否安装 AP 设施，on 表示安装，off 表示不安装。zEC12 或更新 CPU 模型默认为 ``ap=on``。如果为客户机配置了 vfio-ap 设备（``-device vfio-ap,sysfsdev=$path``），则客户机必须安装 AP 设施，否则无法启动。

2. 查询配置信息（QCI）设施

   客户机上运行的 AP 总线使用 QCI 设施查询 AP 设施的配置。只有主机系统安装了 QCI 设施时，此设施才可用。该功能为 s390 专用，在 QEMU 命令行中表示为 -cpu 选项的参数::

      qemu-system-s390x -cpu $model,apqci=on|off


   其中：

      ``$model``
        是为客户机定义的 CPU 模型。

      ``apqci=on|off``
        指示是否安装 QCI 设施，on 表示安装，off 表示不安装。zEC12 或更新 CPU 模型默认为 ``apqci=on``；更旧的模型不安装 QCI。

        如果安装了 QCI（``apqci=on``）但未安装 AP 设施（``ap=off``），会记录错误消息，但仍允许客户机启动。没有 AP 设施时安装 QCI 没有意义，因此这被视为无效配置。

        如果未安装 QCI 设施，客户机上运行的 AP 总线将无法检测 APQI 大于 15 的 APQN。

3. 辅助处理器设施测试（APFT）设施

   客户机上运行的 AP 总线使用 APFT 设施测试给定 AP 队列可用的 AP 设施。只有主机系统安装了 APFT 设施时，此设施才可用。该功能为 s390 专用，在 QEMU 命令行中表示为 -cpu 选项的参数::

      qemu-system-s390x -cpu $model,apft=on|off


   其中：

      ``$model``
        是为客户机定义的 CPU 模型（未指定时默认为主机系统的模型）。

      ``apft=on|off``
        指示是否安装 APFT 设施，on 表示安装，off 表示不安装。zEC12 及更新 CPU 模型默认为 ``apft=on``；更旧的模型不安装 APFT。

        如果安装了 APFT（``apft=on``）但未安装 AP 设施（``ap=off``），会记录错误消息，但仍允许客户机启动。没有 AP 设施时安装 APFT 没有意义，因此这被视为无效配置。

        关闭 APFT 同样没有意义，因为没有它，客户机上运行的 AP 总线就无法检测 CEX4 及更新设备。由于只有 CEX4 及更新设备支持客户机使用，因此未安装 APFT 就启动的客户机无法访问任何 AP 设备。

向运行中的客户机热插入 vfio-ap 设备
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

虚拟机的 ap-bus 只能连接一个 vfio-ap 设备，因此仅当总线上尚未通过 QEMU 命令行或此前热插入操作连接 vfio-ap 设备时，才可以热插入 vfio-ap 设备。

要热插入 vfio-ap 设备，请使用 QEMU 的 ``device_add`` 命令::

    (qemu) device_add vfio-ap,sysfsdev="$path-to-mdev",id="$id"


其中，``$path-to-mdev`` 指定中介设备的绝对路径，该设备已分配了供客户机使用的 AP 资源。``$id`` 是可选 id 参数的名称值。

注意，在 Linux 客户机上，AP 总线随后执行周期性扫描时，才会在 ``/sys/bus/ap/devices`` 目录中创建设备，因此客户机能够访问 AP 设备之前可能有短暂延迟。

以下情况会使命令失败：

* 虚拟机的 ap-bus 已连接一个 vfio-ap 设备。

* 未启用控制客户机访问 AP 设施的 CPU 模型功能（参见上一节中的“CPU 模型功能”部分）。

从运行中的客户机热拔除 vfio-ap 设备
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

如果已通过 QEMU 命令行或此前热插入操作将 vfio-ap 设备连接到虚拟机的 ap-bus，则可以从运行中的 KVM 客户机拔除该设备。

要热拔除 vfio-ap 设备，请使用 QEMU 的 ``device_del`` 命令::

    (qemu) device_del "$id"


其中，``$id`` 与创建设备时指定的 ID 相同。

在 Linux 客户机上，AP 总线随后执行周期性扫描时，才会从客户机的 ``/sys/bus/ap/devices`` 目录移除 AP 设备，因此设备不再能被客户机访问之前可能有短暂延迟。

如果 ``device_del`` 命令指定的 ``$path-to-mdev`` 与 vfio-ap 设备连接到虚拟机 ap-bus 时指定的值不匹配，命令将失败。

示例：为三个 Linux 客户机配置 AP 矩阵
-----------------------------------------------------

下面通过示例说明如何让 KVM 客户机访问 AP 设施。此示例展示如何配置三个客户机，使在客户机中执行 lszcrypt 命令时输出如下：

客户机 1::

  CARD.DOMAIN TYPE  MODE
  ------------------------------
  05          CEX5C CCA-Coproc
  05.0004     CEX5C CCA-Coproc
  05.00ab     CEX5C CCA-Coproc
  06          CEX5A Accelerator
  06.0004     CEX5A Accelerator
  06.00ab     CEX5C CCA-Coproc


客户机 2::

  CARD.DOMAIN TYPE  MODE
  ------------------------------
  05          CEX5A Accelerator
  05.0047     CEX5A Accelerator
  05.00ff     CEX5A Accelerator


客户机 3::

  CARD.DOMAIN TYPE  MODE
  ------------------------------
  06          CEX5A Accelerator
  06.0047     CEX5A Accelerator
  06.00ff     CEX5A Accelerator


步骤如下：

1. 在 Linux 主机上安装 vfio_ap 模块。vfio_ap 模块的依赖链为：

   * iommu
   * s390
   * zcrypt
   * vfio
   * vfio_mdev
   * vfio_mdev_device
   * KVM

   要构建 vfio_ap 模块，内核构建配置必须选择以下 Kconfig 项：

   * IOMMU_SUPPORT
   * S390
   * ZCRYPT
   * S390_AP_IOMMU
   * VFIO
   * VFIO_MDEV
   * VFIO_MDEV_DEVICE
   * KVM

   如果使用 make menuconfig，请选择以下选项构建 vfio_ap 模块::
     -> Device Drivers
        -> IOMMU Hardware Support
           select S390 AP IOMMU Support
        -> VFIO Non-Privileged userspace driver framework
           -> Mediated device driver framework
              -> VFIO driver for Mediated devices
     -> I/O subsystem
        -> VFIO support for AP devices


2. 隔离供三个客户机使用的 AP 队列，确保主机不能访问它们。要将 AP 队列 05.0004、05.0047、05.00ab、05.00ff、06.0004、06.0047、06.00ab 和 06.00ff 保留给 vfio_ap 设备驱动使用，必须按以下方式从默认队列驱动池中移除对应 APQN::

      echo -5,-6 > /sys/bus/ap/apmask

      echo -4,-0x47,-0xab,-0xff > /sys/bus/ap/aqmask


   这会使 AP 队列 05.0004、05.0047、05.00ab、05.00ff、06.0004、06.0047、06.00ab 和 06.00ff 绑定到 vfio_ap 设备驱动。vfio_ap 设备驱动的 sysfs 目录中现在包含指向其所绑定 AP 队列设备的符号链接::

     /sys/bus/ap
     ... [drivers]
     ...... [vfio_ap]
     ......... [05.0004]
     ......... [05.0047]
     ......... [05.00ab]
     ......... [05.00ff]
     ......... [06.0004]
     ......... [06.0047]
     ......... [06.00ab]
     ......... [06.00ff]


   请记住，只有类型 10 及更新适配器（即 CEX4 及后续型号）才能绑定到 vfio_ap 设备驱动。这是为了简化实现，避免为支持即将在相对不久的将来退役、且缺少旧系统用于测试的旧设备，给设计增加不必要的复杂性。

   因此，管理员必须注意，仅隔离能够绑定到 vfio_ap 设备驱动的 AP 队列。给定 AP 队列设备的设备类型可以从父卡的 sysfs 目录读取。例如，查看队列 05.0004 的硬件类型::

     cat /sys/bus/ap/devices/card05/hwtype


   hwtype 必须为 10 或更高（CEX4 或更新），才能绑定到 vfio_ap 设备驱动。

3. 创建中介设备，用于配置三个客户机的 AP 矩阵，并为客户机提供访问 vfio_ap 驱动的接口::

     /sys/devices/vfio_ap/matrix/
     ... [mdev_supported_types]
     ...... [vfio_ap-passthrough] (passthrough mediated matrix device type)
     ......... create
     ......... [devices]


   为三个客户机创建中介设备::

       uuidgen > create
       uuidgen > create
       uuidgen > create


   或者

   ::

       echo $uuid1 > create
       echo $uuid2 > create
       echo $uuid3 > create


   这会在 [devices] 子目录中创建三个中介设备，以创建它们时使用的 UUID 命名。将它们称为 $uuid1、$uuid2 和 $uuid3，创建后的 sysfs 目录结构如下::

     /sys/devices/vfio_ap/matrix/
     ... [mdev_supported_types]
     ...... [vfio_ap-passthrough]
     ......... [devices]
     ............ [$uuid1]
     ............... assign_adapter
     ............... assign_control_domain
     ............... assign_domain
     ............... matrix
     ............... unassign_adapter
     ............... unassign_control_domain
     ............... unassign_domain

     ............ [$uuid2]
     ............... assign_adapter
     ............... assign_control_domain
     ............... assign_domain
     ............... matrix
     ............... unassign_adapter
     ............... unassign_control_domain
     ............... unassign_domain

     ............ [$uuid3]
     ............... assign_adapter
     ............... assign_control_domain
     ............... assign_domain
     ............... matrix
     ............... unassign_adapter
     ............... unassign_control_domain
     ............... unassign_domain


4. 管理员现在需要为中介设备 $uuid1（客户机 1）、$uuid2（客户机 2）和 $uuid3（客户机 3）配置矩阵。

   客户机 1 的矩阵配置方式如下::

      echo 5 > assign_adapter
      echo 6 > assign_adapter
      echo 4 > assign_domain
      echo 0xab > assign_domain


   类似地，可使用 assign_control_domain sysfs 文件分配控制域。

   如果在配置适配器、域或控制域时出错，可以使用 ``unassign_xxx`` 接口取消对应分配。

   显示客户机 1 的矩阵配置::

         cat matrix


   输出以 ``xx.yyyy`` 格式显示 APQN，其中 xx 为适配器编号，yyyy 为域号。客户机 1 的输出如下::

         05.0004
         05.00ab
         06.0004
         06.00ab


   客户机 2 的矩阵配置方式如下::

      echo 5 > assign_adapter
      echo 0x47 > assign_domain
      echo 0xff > assign_domain


   客户机 3 的矩阵配置方式如下::

      echo 6 > assign_adapter
      echo 0x47 > assign_domain
      echo 0xff > assign_domain


5. 启动客户机 1::

   /usr/bin/qemu-system-s390x ... -cpu host,ap=on,apqci=on,apft=on -device vfio-ap,sysfsdev=/sys/devices/vfio_ap/matrix/$uuid1 ...


7. 启动客户机 2::

   /usr/bin/qemu-system-s390x ... -cpu host,ap=on,apqci=on,apft=on -device vfio-ap,sysfsdev=/sys/devices/vfio_ap/matrix/$uuid2 ...


7. 启动客户机 3::

   /usr/bin/qemu-system-s390x ... -cpu host,ap=on,apqci=on,apft=on -device vfio-ap,sysfsdev=/sys/devices/vfio_ap/matrix/$uuid3 ...


客户机关机后，可以移除中介矩阵设备。

继续使用本例，移除中介矩阵设备 $uuid1::

   /sys/devices/vfio_ap/matrix/
   ... [mdev_supported_types]
   ...... [vfio_ap-passthrough]
   ......... [devices]
   ............ [$uuid1]
   ............... remove


   echo 1 > remove


这会移除该 mdev 矩阵设备的全部 sysfs 结构，包括 mdev 设备本身。要重新创建并配置该 mdev 矩阵设备，必须重新执行从步骤 3 开始的全部步骤。注意，如果使用此 mdev 的客户机仍在运行，移除会失败。

不必移除 mdev 矩阵设备，但如果在 Linux 主机剩余运行周期内都不会有客户机使用它，可以将其移除。移除后，还可能需要重新配置保留给默认驱动使用的适配器池和队列池。

限制
-----------

* KVM/内核接口无法阻止将某个队列的 APQN 恢复到默认驱动池，即使该队列仍分配给正在被客户机使用的中介设备。管理员必须确保该 APQN 没有分配给任何正在被客户机使用的中介设备，否则主机将能够访问 AP 队列设备的私有数据，例如专为客户机配置的私钥。

* 目前不支持在运行中的客户机使用中介矩阵设备时，动态分配或取消分配其 AP 资源，参见上文 `为 Linux 客户机配置 AP 矩阵`_。

* 不支持使用 AP 设备的客户机实时迁移。如果客户机正在使用 AP 设备，则迁移前必须拔除为其配置的 vfio-ap 设备（参见上文 `从运行中的客户机热拔除 vfio-ap 设备`_）。
