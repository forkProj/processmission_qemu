Arm CPU 功能
================

CPU 功能是支持该功能的 CPU 类型可选择实现或不实现的可选特性。在 QEMU 中，可选 CPU 功能具有对应的布尔 CPU 属性：启用表示实现该功能，禁用表示不实现。Arm CPU 功能的一个例子是性能监控单元（PMU）。Cortex-A15 和 Cortex-A57 分别实现 ARMv7-A 和 ARMv8-A 架构参考手册，两者都可以选择实现 PMU。例如，要使用不带 PMU 的 Cortex-A15，应在 QEMU 命令行的 ``-cpu`` 参数中包含 ``pmu=off``，即 ``-cpu cortex-a15,pmu=off``。

并非所有 CPU 类型都支持全部可选 CPU 功能，因此 CPU 属性是否存在取决于 CPU 类型。例如，实现 ARMv8-A 架构参考手册的 CPU 可以选择支持 AArch32 功能，可通过禁用 ``aarch64`` CPU 属性来启用它。不实现 ARMv8-A 的 Cortex-A15 等 CPU 类型则没有 ``aarch64`` 属性。

QEMU 对某些 CPU 功能的支持可能有限：可能只支持部分功能，或只在特定配置下支持。例如，禁用后可启用可选 AArch32 功能的 ``aarch64`` CPU 功能，仅能在 TCG 和 KVM 加速器上设为 ``off``；而在 KVM 下，只有主机 CPU 类型支持以 AArch32 运行客户机时，才能设为 ``off``。

KVM 专用的 CPU 功能以 "kvm-" 为前缀，详见“KVM VCPU 功能”。

探测 CPU 功能
===================

可使用 QMP 命令 ``query-cpu-model-expansion``，确定给定 CPU 类型有哪些可用且能工作的功能。以下示例使用 ``scripts/qmp/qmp-shell`` 发出 QMP 命令，用法参见该脚本顶部注释块。

1. 确定 ``max`` CPU 类型提供哪些功能（注意，这里使用 qemu-system-aarch64 启动 QEMU，因此 ``max`` 实现 ARMv8-A 参考手册）::

      (QEMU) query-cpu-model-expansion type=full model={"name":"max"}
      { "return": {
        "model": { "name": "max", "props": {
        "sve1664": true, "pmu": true, "sve1792": true, "sve1920": true,
        "sve128": true, "aarch64": true, "sve1024": true, "sve": true,
        "sve640": true, "sve768": true, "sve1408": true, "sve256": true,
        "sve1152": true, "sve512": true, "sve384": true, "sve1536": true,
        "sve896": true, "sve1280": true, "sve2048": true
      }}}}


可以看到，``max`` CPU 类型具有 ``pmu``、``aarch64``、``sve`` 和许多 ``sve<N>`` 功能。所有功能均为 ``true``，因此都已启用。（``sve<N>`` 功能对应所有可选 SVE 向量长度，参见“SVE CPU 属性”。TCG 可以支持全部 SVE 向量长度；使用 KVM 时，即使支持 SVE，通常也只支持少数长度。）

(2) 尝试禁用 PMU::

      (QEMU) query-cpu-model-expansion type=full model={"name":"max","props":{"pmu":false}}
      { "return": {
        "model": { "name": "max", "props": {
        "sve1664": true, "pmu": false, "sve1792": true, "sve1920": true,
        "sve128": true, "aarch64": true, "sve1024": true, "sve": true,
        "sve640": true, "sve768": true, "sve1408": true, "sve256": true,
        "sve1152": true, "sve512": true, "sve384": true, "sve1536": true,
        "sve896": true, "sve1280": true, "sve2048": true
      }}}}


可以看到操作成功，``pmu`` 现在为 ``false``。

(3) 尝试禁用 ``aarch64``，从而启用 AArch32 CPU 功能::

      (QEMU) query-cpu-model-expansion type=full model={"name":"max","props":{"aarch64":false}}
      {"error": {
       "class": "GenericError", "desc":
       "'aarch64' feature cannot be disabled unless KVM is enabled and 32-bit EL1 is supported"
      }}


看来此功能受限于当前不具备的配置。

(4) 禁用 ``sve``，观察所有可选 SVE 向量长度的变化::

      (QEMU) query-cpu-model-expansion type=full model={"name":"max","props":{"sve":false}}
      { "return": {
        "model": { "name": "max", "props": {
        "sve1664": false, "pmu": true, "sve1792": false, "sve1920": false,
        "sve128": false, "aarch64": true, "sve1024": false, "sve": false,
        "sve640": false, "sve768": false, "sve1408": false, "sve256": false,
        "sve1152": false, "sve512": false, "sve384": false, "sve1536": false,
        "sve896": false, "sve1280": false, "sve2048": false
      }}}}


符合预期，它们现在都为 ``false``。

(5) 尝试探测 Cortex-A15 CPU 类型的功能::

      (QEMU) query-cpu-model-expansion type=full model={"name":"cortex-a15"}
      {"return": {"model": {"name": "cortex-a15", "props": {"pmu": true}}}}


只有 ``pmu`` CPU 功能可用。

关于 CPU 功能依赖关系
-------------------------------------

功能之间可能存在依赖。因此，逐个更改功能可能不会报错，但尝试一次更改所有功能时，可能因处理顺序而报错。也可能一次更改所有功能时，由于其他功能满足了依赖而不报错，但单独更改同一功能却会报错。因此，调用方应始终尝试一次提交全部所需修改，以确保整个组合有效。

关于 CPU 模型和 KVM
-------------------------------

具名 CPU 模型通常不能与 KVM 配合使用。少数情况下可以，例如在 Seattle 主机上使用 ``cortex-a57`` 具名 CPU 模型与 KVM，但大多数情况下，启用 KVM 后必须使用 ``host`` CPU 类型。这意味着向客户机提供与主机 CPU 类型相同的全部功能。因此，``host`` CPU 类型默认应启用主机具备的所有功能。实际上，使用 ``host`` 类型时允许禁用主机已有功能，多少有些奇怪；但在缺少 CPU 模型的情况下，如果希望启动未启用全部主机 CPU 功能的客户机，这是目前最佳做法。

启用 KVM 也会影响 ``query-cpu-model-expansion`` QMP 命令。不仅像“探测 CPU 功能”的示例 (3) 那样影响特定功能，还会影响哪些 CPU 类型可以展开。启用 KVM 时，只能展开 ``max``、``host`` 和当前 CPU 类型。由于无法预知所有能与 KVM 配合的 CPU 类型，这一限制是必要的，但也使用户存在遇到意外错误的小概率风险。例如，如前所述，在 Seattle 主机上，启用 KVM 时 ``cortex-a57`` 也是有效类型。用户可能将 ``host`` 作为当前类型，然后尝试查询 ``cortex-a57``，但此查询会因限制而失败。不过，这通常不是问题，因为管理层和用户长期以来都倾向于在 KVM 下使用 ``host``。此外，如果 Seattle 主机上启用 KVM 的 QEMU 实例正在使用 ``cortex-a57``，则查询 ``cortex-a57`` 可以成功。

使用 CPU 功能
==================

确定给定 CPU 类型有哪些可用且受支持的功能后，可以在 QEMU 命令行中配合该 CPU 类型选择性启用或禁用功能::

  $ qemu-system-aarch64 -M virt -cpu max,pmu=off,sve=on,sve128=on,sve256=on


上例为 ``max`` CPU 类型禁用 PMU，并启用前两种 SVE 向量长度。注意，``sve=on`` 实际并非必需，因为前面探测 ``max`` 时已经看到 ``sve`` 默认启用。此外，从探测的默认值看，似乎应该禁用许多 SVE 向量长度，而不只是启用所需的两种。实际并非如此，因为逐项禁用许多长度会使命令行过于冗长，``sve<N>`` CPU 属性具有特殊语义，参见“SVE CPU 属性解析语义”。

KVM VCPU 功能
=================

KVM VCPU 功能是 KVM 专用的 CPU 功能，例如半虚拟化功能或启用 CPU 虚拟化扩展的功能。其 CPU 属性仅在启用 KVM 时可用，名称以 "kvm-" 为前缀。KVM VCPU 功能可以与其他 CPU 功能一样进行探测、启用和禁用。以下列出这些功能及其说明。

``kvm-no-adjvtime``
  默认禁用 kvm-no-adjvtime，也就是说默认启用虚拟时间调整，并非不调整 vtime。

  启用虚拟时间调整后，虚拟机每次恢复运行时，都会更新 VCPU 虚拟计数器，以确保不计入停止的时间。只要客户机操作系统和应用使用虚拟计数器计时，这就能避免意外的时间跳变。但它也会使虚拟计数器和物理计数器逐渐产生差异。基于虚拟计数器的计时，看起来会落后于未扣除虚拟机停止时间的计时。客户机可按需将其虚拟计数器与其他时间源重新同步。

  启用 kvm-no-adjvtime 可禁用虚拟时间调整，同时恢复旧版（5.0 之前）的行为。

``kvm-steal-time``
  从 v5.2 起，当启用 KVM、此功能受支持且客户机为 64 位时，默认启用 kvm-steal-time。

  启用后，64 位客户机可统计由于主机未调度对应 VCPU 线程而使 CPU 未运行的时间。这些统计信息可能影响客户机调度器行为，也可能提供给客户机用户空间。

``kvm-psci-version``
  设置 KVM 向客户机提供的电源状态协调接口（PSCI）固件 ABI 版本。默认使用 KVM 已知的最新版本（在 Linux v6.13 中为 PSCI v1.3）。

  只有需要将虚拟机迁移到运行较旧内核、无法识别当前主机内核默认 PSCI 版本的主机时，才需要设置此属性。

  当前有效值为 0.1、0.2、1.0、1.1、1.2 和 1.3。

TCG VCPU 功能
=================

TCG VCPU 功能是 TCG 专用的 CPU 功能。以下列出这些功能及其说明。

``pauth``
  完整启用或禁用 ``FEAT_Pauth``。

``pauth-impdef``
  启用 ``pauth`` 时，选择 QEMU 定义的实现算法。

``pauth-qarma3``
  启用 ``pauth`` 时，选择架构规定的 QARMA3 算法。

``pauth-qarma5``
  启用 ``pauth`` 时，选择架构规定的 QARMA5 算法。

如果未启用 ``pauth-impdef``、``pauth-qarma3`` 或 ``pauth-qarma5``，则使用 QEMU 的 impdef 算法。架构规定的 QARMA5 和 QARMA3 算法具有良好的密码学性质，但模拟可能较慢。QEMU 使用的 impdef 算法不具备密码学性质，但速度快得多。

SVE CPU 属性
==================

SVE CPU 属性有两类：``sve`` 和 ``sve<N>``。前者用于启用或禁用整个 SVE 功能，就像 ``pmu`` 属性完整启用或禁用 PMU 一样。后者用于启用或禁用特定向量长度，其中 ``N`` 是以位为单位的长度。``sve<N>`` 属性具有特殊依赖和约束，见下文“SVE CPU 属性的依赖和约束”。此外，由于希望默认启用所有支持的向量长度，为避免命令行过于冗长（为所有不需要的 ``N`` 都填写 ``sve<N>=off``），我们提供了“SVE CPU 属性解析语义”中列出的解析规则。

SVE CPU 属性的依赖和约束
---------------------------------------------

  1) 启用 ``sve`` 时，必须至少启用一种向量长度。

  2) 如果启用向量长度 ``N``，则在启用 KVM 时，所有更小且主机支持的向量长度也必须启用。未启用 KVM 时，只要求启用所有更小且为二的幂的向量长度。例如，使用 KVM 时，若主机支持最高 512 位的全部长度（128、256、384、512），则启用 ``sve512`` 后，128、256 和 384 位长度也必须启用。不使用 KVM 时，则不要求启用 384 位长度。

  3) 启用 KVM 时，只能启用主机 CPU 类型支持的向量长度。如果主机不支持 SVE，则不能启用任何 ``sve*`` 属性。

SVE CPU 属性解析语义
----------------------------------

  1) 如果禁用 SVE（``sve=off``），哪些 SVE 向量长度被启用或禁用对客户机无关紧要，因为整个 SVE 功能被禁用，也就禁用了客户机的全部向量长度。不过，QEMU 仍会跟踪用户提供的 ``sve<N>`` 属性。如果稍后提供 ``sve=on``，则客户机仅获得已启用的长度。如果未提供 ``sve=on``，但存在显式启用的向量长度，则产生错误。

  2) 如果启用 SVE（``sve=on``），但未提供任何 ``sve<N>`` 属性，则启用所有受支持的向量长度。不使用 KVM 时，这包括非二的幂的长度；使用 KVM 时，则包括主机处理器支持的全部长度。

  3) 如果启用了 SVE，尝试禁用最后一种已启用的向量长度时会产生错误（参见“SVE CPU 属性的依赖和约束”的约束 (1)）。

  4) 如果显式启用了一种或多种向量长度，同时显式禁用了最大已启用长度所依赖的至少一种长度，则产生错误（参见“SVE CPU 属性的依赖和约束”的约束 (2)）。

  5) 启用 KVM 时，如果主机不支持 SVE，尝试启用任意 ``sve*`` 属性都会产生错误（参见“SVE CPU 属性的依赖和约束”的约束 (3)）。

  6) 启用 KVM 时，如果主机支持 SVE，尝试启用任何主机不支持的向量长度都会产生错误（参见“SVE CPU 属性的依赖和约束”的约束 (3)）。

  7) 如果一个或多个 ``sve<N>`` 属性设为 ``off``，但没有任何 ``sve<N>`` 属性设为 ``on``，则禁用指定长度，未指定长度默认保持启用。未启用 KVM 时，禁用某个二的幂长度还会禁用所有更大的长度。启用 KVM 时，禁用任意受支持长度都会禁用所有更大的长度（参见“SVE CPU 属性的依赖和约束”的约束 (2)）。

  8) 如果一个或多个 ``sve<N>`` 属性设为 ``on``，则启用这些长度，所有未指定长度默认禁用。但“SVE CPU 属性的依赖和约束”中约束 (2) 要求的长度除外，即使未显式启用，也会自动启用。

  9) 如果先禁用 SVE（``sve=off``），从而允许显式禁用全部向量长度（避免“SVE CPU 属性解析语义”的 (3) 所述错误），再提供 ``sve=on``，则会产生错误。要避免此错误，必须在启用 SVE 前先启用至少一种向量长度。

  10) 启用 SVE（通过 ``sve=on`` 或默认设置）会启用 CPU 支持的全部 SVE 子功能（例如也可能启用 SVE2）。通常没有更底层的控制项用于禁用特定 SVE 子功能。

  11) 禁用 SVE 不会自动禁用 SME。要同时禁用两者，必须使用 ``sve=off,sme=off``。特别是对于 ``max`` CPU，仅使用 ``sve=off`` 会得到只具有 SME 的 CPU（因此仍具有 SVE 向量寄存器）。大多数用户应同时禁用两者。

SVE CPU 属性示例
-------------------------

  1) 禁用 SVE 和 SME::

     $ qemu-system-aarch64 -M virt -cpu max,sve=off,sme=off


  2) 为 ``max`` CPU 类型隐式启用所有向量长度::

     $ qemu-system-aarch64 -M virt -cpu max


  3) 启用 KVM 时，使用 ``host`` CPU 类型隐式启用主机支持的全部向量长度::

     $ qemu-system-aarch64 -M virt,accel=kvm -cpu host

  4) 仅启用 128 位向量长度::

     $ qemu-system-aarch64 -M virt -cpu max,sve128=on


  5) 禁用 512 位向量长度及所有更大长度，因为 512 是二的幂。这会使所有更小、未初始化的长度（128、256、384）默认启用::

     $ qemu-system-aarch64 -M virt -cpu max,sve512=off

  6) 启用 128、256 和 512 位向量长度::

     $ qemu-system-aarch64 -M virt -cpu max,sve128=on,sve256=on,sve512=on


  7) 与 (6) 相同，但由于启用 512 位长度要求启用 128 和 256 位长度，因此让它们自动启用::

     $ qemu-system-aarch64 -M virt -cpu max,sve512=on

  8) 与 (7) 相同，但先禁用 SVE，再重新启用::

     $ qemu-system-aarch64 -M virt -cpu max,sve=off,sve512=on,sve=on


  9) 强制触发与最后一种向量长度相关的错误::

     $ qemu-system-aarch64 -M virt -cpu max,sve128=off
     $ qemu-system-aarch64 -M virt -cpu max,sve=off,sve128=off,sve=on


SVE CPU 属性建议
--------------------------------

“SVE CPU 属性示例”展示了多种选择向量长度的方式，开发者可能借此避免命令行过于冗长。不过，建议显式启用每种所需长度。因此，只有示例 (1)、(4) 和 (6) 展示了推荐用法。

SME CPU 属性示例
-------------------------

  1) 禁用 SME::

     $ qemu-system-aarch64 -M virt -cpu max,sme=off


  2) 为 ``max`` CPU 类型隐式启用所有向量长度::

     $ qemu-system-aarch64 -M virt -cpu max


  3) 仅启用 256 位向量长度::

     $ qemu-system-aarch64 -M virt -cpu max,sme256=on


  3) 启用 256 和 1024 位向量长度::

     $ qemu-system-aarch64 -M virt -cpu max,sme256=on,sme1024=on


  4) 禁用 512 位向量长度。这会使 ``max`` 支持的所有其他长度（128、256、1024 和 2048）默认启用::

     $ qemu-system-aarch64 -M virt -cpu max,sve512=off

SVE 用户模式默认向量长度属性
--------------------------------------------

对于 qemu-aarch64，CPU 属性 ``sve-default-vector-length=N`` 对应 Linux 内核参数文件 ``/proc/sys/abi/sve_default_vector_length``。默认长度 ``N`` 以字节为单位，必须介于 16 和 8192 之间。未指定时，默认向量长度为 64。

如果默认长度大于已启用的最大向量长度，则实际长度会缩短。注意，QEMU 支持的最大向量长度为 256。

如果将此属性设为 ``-1``，则默认向量长度设为可能的最大长度。

SME CPU 属性
==================

SME CPU 属性与 SVE 属性十分相似：``sme`` 用于启用或禁用整个 SME 功能，``sme<N>`` 用于启用或禁用特定向量长度。``sme_fa64`` 用于启用或禁用 ``FEAT_SME_FA64``，允许在流式 SVE 模式启用时执行“完整 a64”指令集。

KVM 目前不支持 SME。

启用 ``sme`` 时必须至少启用一种向量长度，且所有长度必须为 2 的幂。QEMU 支持的最大向量长度为 2048 位。除此以外，SME 支持的向量长度集合没有其他约束。

与 SVE 一样，``sme=on`` 启用 CPU 支持的全部 SME 子功能（例如也可能启用 SME2），没有更底层的控制项用于细粒度禁用特定 SME 子功能。

SME 用户模式默认向量长度属性
--------------------------------------------

对于 qemu-aarch64，CPU 属性 ``sme-default-vector-length=N`` 对应 Linux 内核参数文件 ``/proc/sys/abi/sme_default_vector_length``。默认长度 ``N`` 以字节为单位，必须介于 16 和 8192 之间。未指定时，默认向量长度为 32。

与 ``sve-default-vector-length`` 一样，如果默认长度大于已启用的最大长度，则实际向量长度会缩短。如果此属性设为 ``-1``，则默认长度设为可能的最大长度。

RME CPU 属性
==================

QEMU 的 RME 支持目前处于实验阶段，只支持 CPU 本身的 RME，不支持 SMMU 或 GIC 中的 RME。通过 CPU 属性 ``x-rme`` 启用此功能，``x-`` 前缀用于提醒其实验性质，默认禁用。

未来某个 QEMU 版本将改变 RME 的启用方式，不另行通知，也不保证向后兼容。

RME 第 0 级 GPT 大小属性
-----------------------------

为帮助固件开发者测试不同的可能 CPU 配置，可使用 ``x-l0gptsz=S`` 指定编码到 ``GPCCR_EL3.L0GPTSZ`` 的值。该只读字段指定第 0 级粒度保护表的大小。``S`` 的有效值为 30、34、36 和 39，默认值为 30。

与 ``x-rme`` 一样，``x-l0gptsz`` 属性可能在未来某个 QEMU 版本中重命名或移除。
