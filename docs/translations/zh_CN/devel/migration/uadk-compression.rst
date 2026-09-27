==================================
用户空间加速器开发套件（UADK）压缩
==================================
UADK 是通用用户空间加速器框架，通过共享虚拟寻址（SVA）
为密码和压缩算法硬件加速提供统一编程接口。

UADK 包含统一／面向用户空间访问的加速器框架（UACCE），
使不同厂商支持 SVA 的硬件加速器能够接入 UADK。

目前海思鲲鹏硬件加速器已注册到 UACCE。
用户可通过 UADK 让硬件加速器而非 CPU 执行密码和压缩算法，
释放 CPU 算力并提高计算性能。

https://github.com/Linaro/uadk/tree/master/docs

UADK 框架
=========
UADK 由 UACCE、厂商驱动和算法层组成。
要求加速器支持 SVA，操作系统支持 IOMMU 和 SVA。
不同厂商的加速器通过各自内核驱动向 UACCE 注册为不同字符设备，
用户通过用户态操作这些设备访问硬件。

::

          +----------------------------------+
          |                应用              |
          +----+------------------------+----+
               |                        |
               |                        |
       +-------+--------+       +-------+-------+
       |   调度器       |       | 算法库        |
       +-------+--------+       +-------+-------+
               |                         |
               |                         |
               |                         |
               |                +--------+------+
               |                | 厂商驱动      |
               |                +-+-------------+
               |                  |
               |                  |
            +--+------------------+--+
            |         libwd          |
    用户态  +----+-------------+-----+
    --------------------------------------------------
    内核态    +--+-----+   +------+
              | uacce  |   | smmu |
              +---+----+   +------+
                  |
              +---+------------------+
              | 厂商内核驱动         |
              +----------------------+
    --------------------------------------------------
             +----------------------+
             |   硬件加速器         |
             +----------------------+

安装 UADK
---------
构建 UADK
^^^^^^^^^

.. code-block:: shell

    git clone https://github.com/Linaro/uadk.git
    cd uadk
    mkdir build
    ./autogen.sh
    ./configure --prefix=$PWD/build
    make
    make install

未指定 --prefix 时，默认安装到 /usr/local/lib。
如果出现“cannot find -lnuma”，请安装 libnuma-dev。

运行 pkg-config libwd 确认环境正确
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* export PKG_CONFIG_PATH=$PWD/build/lib/pkgconfig
* pkg-config libwd --cflags --libs
  -I/usr/local/include -L/usr/local/lib -lwd

* 按需导出 PKG_CONFIG_PATH。
  安装到 /usr/local/lib 时不需要。

UADK 主机内核要求
-----------------
应确保 Linux 内核支持 UACCE。
至少使用 v5.9，并启用共享虚拟寻址（SVA）。

内核配置
^^^^^^^^

UACCE 可以编译为模块或内置。

以下是在海思鲲鹏平台启用 UACCE 和硬件加速器的示例。

*    CONFIG_IOMMU_SVA_LIB=y
*    CONFIG_ARM_SMMU=y
*    CONFIG_ARM_SMMU_V3=y
*    CONFIG_ARM_SMMU_V3_SVA=y
*    CONFIG_PCI_PASID=y
*    CONFIG_UACCE=y
*    CONFIG_CRYPTO_DEV_HISI_QM=y
*    CONFIG_CRYPTO_DEV_HISI_ZIP=y

确保启用以上全部内核配置。

加速器设备节点权限
------------------
硬件加速器（例如海思鲲鹏 Zip）注册到 UADK 后，会在 dev 目录创建字符设备。
要访问加速器资源，需要向用户授予写权限。

.. code-block:: shell

    $ sudo chmod 777 /dev/hisi_zip-*

在 QEMU 迁移中使用 UADK 压缩
----------------------------
* 确保按上述方法安装 UADK。
* 构建 QEMU 时使用 ``--enable-uadk``。

  例如 configure --target-list=aarch64-softmmu --enable-kvm ``--enable-uadk``

* 在迁移期间启用 UADK 压缩。

  设置 ``migrate_set_parameter multifd-compression uadk``。

UADK 使用 SVA，设备直接访问虚拟内存，因此 SMMUv3 遍历 I/O 页表时
可能发生缺页，影响性能。为减轻影响，
请确保目标虚拟机启动参数包含 ``-mem-prealloc``。

虽然 UADK 和 ZLIB 都基于 deflate，但两者并不完全兼容。
因此迁移时源端和目标端都必须使用 uadk。
