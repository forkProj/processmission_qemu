.. _qom:

====================
QEMU 对象模型（QOM）
====================

.. highlight:: c

QEMU 对象模型提供注册用户可创建类型，并从类型实例化对象的框架，支持：

- 动态类型注册；
- 类型单继承；
- 无状态接口多继承；
- 将内部成员映射为公开属性。

根对象类为 TYPE_OBJECT，提供基本对象方法。

QOM 树
========

QOM 树是一棵组合树，表示构成 QEMU 机器的全部对象。
在 :ref:`QEMU monitor` 运行 ``info qom-tree`` 可以查看，
其中既有机器自身创建的对象，也有用户配置产生的对象。

创建 QOM 类
===========

一个最小设备实现如下：

.. code-block:: c
   :caption: Creating a minimal type

   #include "qdev.h"

   #define TYPE_MY_DEVICE "my-device"

   // No new virtual functions: we can reuse the typedef for the
   // superclass.
   typedef DeviceClass MyDeviceClass;
   typedef struct MyDevice
   {
       DeviceState parent_obj;

       int reg0, reg1, reg2;
   } MyDevice;

   static const TypeInfo my_device_info = {
       .name = TYPE_MY_DEVICE,
       .parent = TYPE_DEVICE,
       .instance_size = sizeof(MyDevice),
   };

   static void my_device_register_types(void)
   {
       type_register_static(&my_device_info);
   }

   type_init(my_device_register_types)


上例通过 TypeInfo 描述一个简单类型，包括父类、实例和类大小，
以及构造／析构钩子。

TYPE_DEVICE 是 QEMU 所有现代设备的父类，
增加设备模型所需方法，包括管理从创建、向客户机呈现到最终 unrealize 的生命周期。

也可以通过 DEFINE_TYPES() 辅助宏注册多个静态类型：

.. code-block:: c

   static const TypeInfo device_types_info[] = {
       {
           .name = TYPE_MY_DEVICE_A,
           .parent = TYPE_DEVICE,
           .instance_size = sizeof(MyDeviceA),
       },
       {
           .name = TYPE_MY_DEVICE_B,
           .parent = TYPE_DEVICE,
           .instance_size = sizeof(MyDeviceB),
       },
   };

   DEFINE_TYPES(device_types_info)


每个类型都有关联的 ObjectClass。
ObjectClass 派生对象动态创建，但每个类型始终只有一个实例，
通常保存该类型实现的虚函数指针表。

object_new() 创建 Object 派生实例。
object_dynamic_cast() 可将 Object 转换为子类或基类。
通常应为 OBJECT_CHECK() 和 OBJECT_CLASS_CHECK() 定义封装宏，
方便转换为具体类型：

.. code-block:: c
   :caption: Typecasting macros

   #define MY_DEVICE_GET_CLASS(obj) \
      OBJECT_GET_CLASS(MyDeviceClass, obj, TYPE_MY_DEVICE)
   #define MY_DEVICE_CLASS(klass) \
      OBJECT_CLASS_CHECK(MyDeviceClass, klass, TYPE_MY_DEVICE)
   #define MY_DEVICE(obj) \
      OBJECT_CHECK(MyDevice, obj, TYPE_MY_DEVICE)


如果 ObjectClass 实现可编译为模块，必须添加 module_obj()，
确保需要该对象时 QEMU 加载模块。

.. code-block:: c

   module_obj(TYPE_MY_DEVICE);


类初始化
--------

初始化对象之前，必须先初始化其类。
所有实例共用一个类对象，按需延迟创建。

首先初始化必要的父类，再将父类对象复制到当前类对象，
当前类额外的存储区域清零。

因此，类自动继承父类已初始化的虚函数指针，其余字段均为零。

全部父类初始化后，调用 TypeInfo::class_init，
让当前类为自己的虚函数提供默认初始化。
例如，可以这样修改前例以覆盖虚函数：

.. code-block:: c
   :caption: Overriding a virtual function

   #include "qdev.h"

   void my_device_class_init(ObjectClass *klass, const void *class_data)
   {
       DeviceClass *dc = DEVICE_CLASS(klass);
       dc->reset = my_device_reset;
   }

   static const TypeInfo my_device_info = {
       .name = TYPE_MY_DEVICE,
       .parent = TYPE_DEVICE,
       .instance_size = sizeof(MyDevice),
       .class_init = my_device_class_init,
   };


增加新虚方法时，需要定义自己的类结构，并向 TypeInfo 添加 .class_size。
每个方法还应提供方便调用的封装函数：

.. code-block:: c
   :caption: Defining an abstract class

   #include "qdev.h"

   typedef struct MyDeviceClass
   {
       DeviceClass parent_class;

       void (*frobnicate) (MyDevice *obj);
   } MyDeviceClass;

   static const TypeInfo my_device_info = {
       .name = TYPE_MY_DEVICE,
       .parent = TYPE_DEVICE,
       .instance_size = sizeof(MyDevice),
       .abstract = true, // or set a default in my_device_class_init
       .class_size = sizeof(MyDeviceClass),
   };

   void my_device_frobnicate(MyDevice *obj)
   {
       MyDeviceClass *klass = MY_DEVICE_GET_CLASS(obj);

       klass->frobnicate(obj);
   }


接口
--------

接口允许有限形式的多继承。
接口实例类似普通类型，但只由类定义，不携带任何状态。
因此，接口实例指针应始终指向不完整类型，确保不能解引用。
例如声明 typedef struct SomethingIf SomethingIf，
以便传递 ``SomethingIf *si``，但不定义 ``struct SomethingIf { ... }``。
对 SomethingIf * 合法的操作只有：将它传给对应 SomethingIfClass 的方法，
或动态转换为实现该接口的对象。

方法
--------

方法是类命名空间内的函数，通常以强类型的第一个参数接收对象实例并操作它。
不操作实例的方法称为类方法。

方法不能重载。ObjectClass 与方法名唯一确定调用目标，
除末尾可变参数外，签名不得变化。

方法始终是虚方法。
在子类的 TypeInfo.class_init 中覆盖后，
通过 OBJECT_GET_CLASS() 获取类的所有使用者都会访问覆盖后的函数。
原函数不会自动调用，由覆盖者决定是否及何时调用被覆盖方法。

首选做法是在覆盖前将原函数值保存在子类中，
类似 Java 的 super.method(...) 和 C# 的 base.method(...)。
这样不必硬编码父类，允许将来调整父类。

.. code-block:: c
   :caption: Overriding a virtual method

   typedef struct MyState MyState;

   typedef void (*MyDoSomething)(MyState *obj);

   typedef struct MyClass {
       ObjectClass parent_class;

       MyDoSomething do_something;
   } MyClass;

   static void my_do_something(MyState *obj)
   {
       // do something
   }

   static void my_class_init(ObjectClass *oc, const void *data)
   {
       MyClass *mc = MY_CLASS(oc);

       mc->do_something = my_do_something;
   }

   static const TypeInfo my_type_info = {
       .name = TYPE_MY,
       .parent = TYPE_OBJECT,
       .instance_size = sizeof(MyState),
       .class_size = sizeof(MyClass),
       .class_init = my_class_init,
   };

   typedef struct DerivedClass {
       MyClass parent_class;

       MyDoSomething parent_do_something;
   } DerivedClass;

   static void derived_do_something(MyState *obj)
   {
       DerivedClass *dc = DERIVED_GET_CLASS(obj);

       // do something here
       dc->parent_do_something(obj);
       // do something else here
   }

   static void derived_class_init(ObjectClass *oc, const void *data)
   {
       MyClass *mc = MY_CLASS(oc);
       DerivedClass *dc = DERIVED_CLASS(oc);

       dc->parent_do_something = mc->do_something;
       mc->do_something = derived_do_something;
   }

   static const TypeInfo derived_type_info = {
       .name = TYPE_DERIVED,
       .parent = TYPE_MY,
       .class_size = sizeof(DerivedClass),
       .class_init = derived_class_init,
   };


另一种方式是通过 object_class_by_name() 获取特定类型的类及其未覆盖方法，
类似 C++ 的 MyClass::method(...)。

DeviceClass.reset 就是一个例子，更多示例见 :ref:`device-life-cycle`。

标准类型声明与定义宏
====================

上述大量代码遵循固定模式和命名约定。
为了减少新类型所需样板代码，提供两组宏，以标准格式生成公共部分。

用 OBJECT_DECLARE 宏族声明类型。
若类不需要虚函数，可用 OBJECT_DECLARE_SIMPLE_TYPE，通常放在头文件中：

.. code-block:: c
   :caption: Declaring a simple type

   OBJECT_DECLARE_SIMPLE_TYPE(MyDevice, MY_DEVICE)


等价于：

.. code-block:: c
   :caption: Expansion from declaring a simple type

   typedef struct MyDevice MyDevice;
   typedef struct MyDeviceClass MyDeviceClass;

   G_DEFINE_AUTOPTR_CLEANUP_FUNC(MyDeviceClass, object_unref)

   #define MY_DEVICE_GET_CLASS(void *obj) \
           OBJECT_GET_CLASS(MyDeviceClass, obj, TYPE_MY_DEVICE)
   #define MY_DEVICE_CLASS(void *klass) \
           OBJECT_CLASS_CHECK(MyDeviceClass, klass, TYPE_MY_DEVICE)
   #define MY_DEVICE(void *obj)
           OBJECT_CHECK(MyDevice, obj, TYPE_MY_DEVICE)

   struct MyDeviceClass {
       DeviceClass parent_class;
   };


struct MyDevice 仍需单独声明。
若需要在类结构中声明虚函数，应使用 OBJECT_DECLARE_TYPE()，
其行为类似 SIMPLE 版本，但不生成 struct MyDeviceClass 定义。

实现类型时使用 OBJECT_DEFINE 宏族。
没有自身虚函数、通过 OBJECT_DECLARE_SIMPLE_TYPE 声明的叶子类，
可以使用 OBJECT_DEFINE_SIMPLE_TYPE：

.. code-block:: c
   :caption: Defining a simple type

   OBJECT_DEFINE_SIMPLE_TYPE(MyDevice, my_device, MY_DEVICE, DEVICE)


等价于：

.. code-block:: c
   :caption: Expansion from defining a simple type

   static void my_device_finalize(Object *obj);
   static void my_device_class_init(ObjectClass *oc, const void *data);
   static void my_device_init(Object *obj);

   static const TypeInfo my_device_info = {
       .parent = TYPE_DEVICE,
       .name = TYPE_MY_DEVICE,
       .instance_size = sizeof(MyDevice),
       .instance_init = my_device_init,
       .instance_finalize = my_device_finalize,
       .class_init = my_device_class_init,
   };

   static void
   my_device_register_types(void)
   {
       type_register_static(&my_device_info);
   }
   type_init(my_device_register_types);


这已足以在类型系统中注册类型，随后需要实现三个标准方法和该类型的其他逻辑。

若类需要自己的虚方法，或需要在类结构保存其他按类共享的状态，
使用 OBJECT_DEFINE_TYPE。
它与 SIMPLE 版本相同，但额外将 class_size 设为类结构大小。

.. code-block:: c
   :caption: Defining a type which needs a class struct

   OBJECT_DEFINE_TYPE(MyDevice, my_device, MY_DEVICE, DEVICE)


若实现一个或多个接口，可以使用 OBJECT_DEFINE_SIMPLE_TYPE_WITH_INTERFACES()
或 OBJECT_DEFINE_TYPE_WITH_INTERFACES()，接收接口类型名称数组。
前者适用于无需类结构的简单叶子类，后者适用于自行定义类结构的情况。

.. code-block:: c
   :caption: Defining a simple type implementing interfaces

   OBJECT_DEFINE_SIMPLE_TYPE_WITH_INTERFACES(MyDevice, my_device,
                                             MY_DEVICE, DEVICE,
                                             { TYPE_USER_CREATABLE },
                                             { NULL })


.. code-block:: c
   :caption: Defining a type implementing interfaces

   OBJECT_DEFINE_TYPE_WITH_INTERFACES(MyDevice, my_device,
                                      MY_DEVICE, DEVICE,
                                      { TYPE_USER_CREATABLE },
                                      { NULL })


不希望类型被实例化时，可使用 OBJECT_DEFINE_ABSTRACT_TYPE()：

.. code-block:: c
   :caption: Defining a simple abstract type

   OBJECT_DEFINE_ABSTRACT_TYPE(MyDevice, my_device,
                               MY_DEVICE, DEVICE)


.. _device-life-cycle:

设备生命周期
============

类初始化不能失败，因此设备另有两个方法处理动态设备创建。
realize 接收 ``Error **``，若无法完成设置，应通过它报告错误。
成功后，设备加入 QOM 树，并对客户机可见。

反向操作是 unrealize，系统不再使用设备时，应在其中完成清理。

所有设备都可由 C 代码实例化，但只有部分设备可通过命令行或监控器动态创建。

同样，只有部分设备可在创建后拔出，并需要显式实现 unrealize。
这由根 DeviceClass 结构中的 user_creatable 决定。
只有 parent_bus 注册了 HotplugHandler 的设备才可拔出。

API 参考
========

完整 API 说明见 :ref:`QOM API<qom-api>` 和 :ref:`QDEV API<qdev-api>`。
