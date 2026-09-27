=================
QEMU 磁盘镜像工具
=================

用法概要
--------

**qemu-img** [*standard options*] *command* [*command options*]

说明
--------

qemu-img 可以离线创建、转换和修改镜像，支持 QEMU 的全部镜像格式。

**警告：** 不要使用 qemu-img 修改正在被虚拟机或其他进程使用的镜像，否则可能损坏镜像。
查询其他进程正在修改的镜像时，也可能遇到不一致状态。

选项
--------

.. program:: qemu-img

标准选项：

.. option:: -h, --help

  显示帮助并退出。

.. option:: -V, --version

  显示版本信息并退出。

.. option:: -T, --trace [[enable=]PATTERN][,events=FILE][,file=FILE]

  .. include:: ../qemu-option-trace.rst.inc

支持以下命令：

.. hxtool-doc:: qemu-img-cmds.hx

命令参数：

*FILENAME* 是磁盘镜像文件名。

*FMT* 是镜像格式，多数情况下会自动检测。支持的格式说明见下文。

*SIZE* 是镜像大小，单位为字节。
支持可选后缀 ``k`` 或 ``K`` （1024 字节）、``M`` （1024k）、``G`` （1024M）
和 T（1024G）；``b`` 会被忽略。

*OUTPUT_FILENAME* 是目标镜像文件名。

*OUTPUT_FMT* 是目标格式。

*OPTIONS* 是以逗号分隔的格式专用选项列表，采用 name=value 形式。
使用 ``-o help`` 查看所选格式支持的选项概览，或参阅下文格式说明。

*SNAPSHOT_PARAM* 用于指定内部快照，格式为
'snapshot.id=[ID],snapshot.name=[NAME]' 或 '[ID_OR_NAME]'。

..
  这里需要新的 program，否则 Sphinx 会报告 -h 同时出现在两个选项列表中。

.. program:: qemu-img-common-opts

.. option:: --object OBJECTDEF

  定义 QEMU 用户可创建对象，属性详见 :manpage:`qemu(1)`。
  最常用的是提供密码或加密密钥的 ``secret``。

.. option:: --image-opts

  将源 *FILENAME* 解释为完整选项字符串，而非普通文件名，与 *-f* 互斥。

.. option:: --target-image-opts

  将 OUTPUT_FILENAME 参数解释为完整选项字符串，而非普通文件名，与 *-O* 互斥。
  当前还必须同时使用 *-n* 跳过镜像创建，此限制未来可能放宽。

.. option:: --force-share (-U)

  以共享模式打开镜像，允许其他 QEMU 进程以写入模式打开它。
  例如，客户机正在使用镜像时，可用 info 子命令查询信息。
  但并发元数据修改等因素可能导致结果不一致。
  此选项仅允许在只读打开镜像时使用。

.. option:: --backing-chain

  枚举镜像后备链中的文件信息，详见下文。

.. option:: -c

  要求压缩目标镜像，仅适用于 qcow、qcow2，以及 streamOptimized 子格式的 vmdk。

  qcow2 可以通过 ``-o compression_type=...`` 指定算法，见下文。

.. option:: -h

  无论是否给出命令，都显示帮助并列出支持的格式。

.. option:: -p

  显示进度条，仅用于 compare、convert 和 rebase。
  对支持该选项的命令，如果未指定 *-p*，
  则在进程收到 ``SIGUSR1`` 或 ``SIGINFO`` 时报告进度。

.. option:: -q

  安静模式，除错误外不打印输出。同时使用 *-q* 和 *-p* 时，不显示进度条。

.. option:: -S SIZE

  转换时，只有连续零字节数达到此值，才创建稀疏区域。
  该值向下取整到 512 字节的倍数，支持 ``k`` 等常见大小后缀。

.. option:: -t CACHE

  设置目标文件的缓存模式。允许值见模拟器 ``-drive cache=...`` 的说明。

.. option:: -T SRC_CACHE

  设置源文件的缓存模式。允许值见模拟器 ``-drive cache=...`` 的说明。

compare 子命令参数：

.. program:: qemu-img-compare

.. option:: -f

  第一个镜像的格式。

.. option:: -F

  第二个镜像的格式。

.. option:: -s

  严格模式，镜像大小或扇区分配不同即判定失败。

convert 子命令参数：

.. program:: qemu-img-convert

.. option:: --bitmaps

  额外复制源镜像顶层的全部持久位图。

.. option:: -n

  跳过目标卷创建。

.. option:: -m

  转换过程的并行协程数。

.. option:: -W

  允许向目标乱序写入，以提升性能。
  仅建议用于宿主设备或其他 raw 块设备等已预分配设备。

.. option:: -C

  尝试通过复制卸载将数据从源镜像移到目标。
  对 NFS 或 iSCSI 等远程数据可能更快，但不会自动将零扇区稀疏化，
  且根据宿主获取分配信息的能力，可能生成完全分配的目标镜像。

.. option:: -r

   转换过程的速率限制。

.. option:: --salvage

  尝试忽略读取时的 I/O 错误。除安静模式 ``-q`` 外，仍会打印错误。
  无法读取的源区域按全零处理。

.. option:: --target-is-zero

  假定读取目标镜像总是得到零。与具有后备文件的目标镜像互斥，
  还必须使用 ``-n`` 跳过创建镜像。

dd 子命令参数：

.. program:: qemu-img-dd

.. option:: bs=BLOCK_SIZE

  定义块大小。

.. option:: count=BLOCKS

  设置要复制的输入块数。

.. option:: if=INPUT

  设置输入文件。

.. option:: of=OUTPUT

  设置输出文件。

.. option:: skip=BLOCKS

  设置跳过的输入块数。

snapshot 子命令参数：

.. program:: qemu-img-snapshot

.. option:: snapshot

  要创建、应用或删除的快照名称。

.. option:: -a

  应用快照，将磁盘恢复到保存状态。

.. option:: -c

  创建快照。

.. option:: -d

  删除快照。

.. option:: -l

  列出镜像中的全部快照，这是默认操作。

命令说明：

.. program:: qemu-img-commands

.. option:: amend [--object OBJECTDEF] [--image-opts] [-p] [-q] [-f FMT] [-t CACHE] [--force] -o OPTIONS FILENAME

  修改镜像 *FILENAME* 的格式专用选项 *OPTIONS*。
  并非所有格式都支持此操作。

  可修改选项取决于镜像格式，但改变后备链关系应使用 ``qemu-img rebase``。

  --force 允许部分不安全操作。
  当前对 -f luks，它允许删除最后一个加密密钥，或覆盖正在使用的密钥。

.. option:: bench [-c COUNT] [-d DEPTH] [-f FMT] [--flush-interval=FLUSH_INTERVAL] [-i AIO] [-n] [--no-drain] [-o OFFSET] [--pattern=PATTERN] [-q] [-s BUFFER_SIZE] [-S STEP_SIZE] [-t CACHE] [-w] [-U] FILENAME

  对镜像运行简单的顺序 I/O 基准测试。指定 ``-w`` 时测试写入，否则测试读取。

  共发出 *COUNT* 个请求，每个 *BUFFER_SIZE* 字节，同时保持 *DEPTH* 个并行请求。
  首个请求从 *OFFSET* 开始，后续每次增加 *STEP_SIZE*；
  未指定步长时使用 *BUFFER_SIZE*。

  写测试指定 *FLUSH_INTERVAL* 后，每当剩余请求数是该值的倍数，
  都会先排空请求队列并执行 flush，再继续写入。
  同时指定 ``--no-drain`` 时，会直接 flush 而不先排空队列。

  ``-i`` 可通过 *AIO* 选择 ``threads``、``native`` 或 ``io_uring`` 后端。

  指定 ``-n`` 时，尽可能使用原生 AIO。
  Linux 上还必须同时指定 ``-t none`` 或 ``-t directsync``。

  写测试默认使用填零缓冲区，可以通过 *PATTERN* 指定其他模式字节。

.. option:: bitmap (--merge SOURCE | --add | --remove | --remove-all | --clear | --enable | --disable)... [-b SOURCE_FILE [-F SOURCE_FMT]] [-g GRANULARITY] [--object OBJECTDEF] [--image-opts | -f FMT] FILENAME [BITMAP]

  对镜像 *FILENAME* 的持久位图执行一个或多个修改。
  大多数操作要求指定 *BITMAP*；``--remove-all`` 操作全部位图，不接受该参数。
  可执行以下修改：

  ``--add`` 创建 *BITMAP*，并启用后续修改记录。

  ``--remove`` 删除 *BITMAP*。

  ``--remove-all`` 删除全部位图。

  ``--clear`` 清空 *BITMAP*。

  ``--enable`` 使 *BITMAP* 开始记录后续修改。

  ``--disable`` 使 *BITMAP* 停止记录后续修改。

  ``--merge`` 将 *SOURCE* 位图内容合并到 *BITMAP*。

  ``-g`` 为 ``--add`` 设置非默认 *GRANULARITY*。
  ``-b`` 和 ``-F`` 为 ``--merge`` 使用的全部 *SOURCE* 位图选择其他源文件。

  查看镜像中的位图可使用 ``qemu-img info``。

.. option:: check [--object OBJECTDEF] [--image-opts] [-q] [-f FMT] [--output=OFMT] [-r [leaks | all]] [-T SRC_CACHE] [-U] FILENAME

  检查镜像 *FILENAME* 的一致性。输出格式 *OFMT* 为 ``human`` 或 ``json``。
  JSON 输出为 QAPI ``ImageCheck`` 对象。

  指定 ``-r`` 后尝试修复问题。``-r leaks`` 仅修复簇泄漏；
  ``-r all`` 修复全部错误，但错误修复或掩盖既有损坏的风险更高。

  只有 ``qcow2``、``qed``、``parallels``、``vhdx``、``vmdk`` 和 ``vdi`` 支持一致性检查。

  镜像一致时退出码为 ``0``，其他退出码表示发现的问题或执行错误：

  0
    检查完成，镜像当前一致。
  1
    因内部错误未完成检查。
  2
    检查完成，镜像已损坏。
  3
    检查完成，存在泄漏簇，但镜像未损坏。
  63
    镜像格式不支持检查。

  指定 ``-r`` 时，镜像状态类退出码反映尝试修复后的状态。
  因此，成功执行 ``-r all`` 后返回 0，与修复前状态无关。

.. option:: commit [--object OBJECTDEF] [--image-opts] [-q] [-f FMT] [-t CACHE] [-b BASE] [-r RATE_LIMIT] [-d] [-p] FILENAME

  将 *FILENAME* 中的修改提交到底层镜像或后备文件。
  后备文件比快照小时，会扩展到相同大小；快照较小时，不会截断后备文件。
  如需匹配较小快照的大小，可以在 commit 成功后自行安全截断。

  成功后会清空 *FILENAME*。如果之后不再需要它、准备删除，
  可以指定 ``-d`` 跳过清空。

  后备链有多层时，可以用 *BASE* 指定提交目标，但它必须属于该链。
  未指定时，使用顶层镜像 *FILENAME* 的直接后备文件。
  注意，提交后 *BASE* 与顶层之间的全部中间镜像都会失效，读取可能返回垃圾数据。
  因此 ``-b`` 隐含 ``-d``，使顶层保持有效。

  ``-r`` 指定提交过程的速率限制。

.. option:: compare [--object OBJECTDEF] [--image-opts] [-f FMT] [-F FMT] [-T SRC_CACHE] [-p] [-q] [-s] [-U] FILENAME1 FILENAME2

  比较两个镜像的内容，可比较不同格式或设置的镜像。

  除非用 ``-f`` 指定 *FILENAME1* 格式、用 ``-F`` 指定 *FILENAME2* 格式，
  否则会自动探测。

  默认情况下，若较大镜像超出的区域只有未分配或全零扇区，大小不同也视为相同。
  如果某扇区在一侧未分配、另一侧全零，也视为相同。
  ``-s`` 启用严格模式，大小不同，或任一扇区只在一侧分配，都会判为不同。

  默认打印比较结果，包括镜像相同，或首个不同字节的位置。
  严格模式下，还可能报告大小不同。

  相同时返回 ``0``，不同时返回 ``1``。
  其他退出码表示执行错误，标准错误输出会包含消息：

  0
    镜像相同，或已打印请求的帮助。
  1
    镜像不同。
  2
    打开镜像失败。
  3
    检查扇区分配失败。
  4
    读取数据失败。

.. option:: convert [--object OBJECTDEF] [--image-opts] [--target-image-opts] [--target-is-zero] [--bitmaps [--skip-broken-bitmaps]] [-U] [-C] [-c] [-p] [-q] [-n] [-f FMT] [-t CACHE] [-T SRC_CACHE] [-O OUTPUT_FMT] [-b BACKING_FILE [-F BACKING_FMT]] [-o OPTIONS] [-l SNAPSHOT_PARAM] [-S SPARSE_SIZE] [-r RATE_LIMIT] [-m NUM_COROUTINES] [-W] FILENAME [FILENAME2 [...]] OUTPUT_FILENAME

  将 *FILENAME* 或其快照 *SNAPSHOT_PARAM* 转换为 *OUTPUT_FMT* 格式的
  *OUTPUT_FILENAME*。可用 ``-c`` 压缩，或用 ``-o`` 指定加密等格式专用选项。

  只有 ``qcow`` 和 ``qcow2`` 支持压缩。压缩数据只能读取；
  如果重写压缩扇区，新数据以非压缩方式写入。

  对 ``qcow`` 等可增长格式，转换还可缩小镜像：
  检测空扇区，并在目标镜像中省略这些区域。

  *SPARSE_SIZE* 指连续全零字节达到多少才创建稀疏区域，默认 4k。
  设为 0 时，不扫描未分配或全零扇区，目标始终完全分配。

  *BACKING_FILE* 可强制创建基于指定底层镜像的写时复制镜像。
  它的内容应与输入镜像的底层镜像相同，但路径、*BACKING_FMT* 格式等可以不同。

  相对路径以 *OUTPUT_FILENAME* 所在目录为基准。

  ``-n`` 跳过目标卷创建。
  对 rbd 等格式，如果卷已经通过 qemu-img 无法表达的环境专用选项创建，这很有用。

  ``-W`` 启用乱序写以提高性能，但只建议用于宿主设备或其他 raw 块设备等预分配设备。
  乱序写不能与压缩镜像创建同时使用。

  *NUM_COROUTINES* 指定转换过程的并行协程数，默认 8。

  ``--bitmaps`` 要求同时复制源中的持久位图。
  如果源中有不一致位图，转换会失败；同时指定 ``--skip-broken-bitmaps``
  可以只复制一致位图。

.. option:: create [-f FMT] [-o FMT_OPTS] [-b BACKING_FILE [-B BACKING_FMT]] [-u] [-q] [--object OBJDEF] FILE [SIZE]

  创建大小为 *SIZE*、格式为 *FMT* 的镜像 *FILE*。
  根据格式，可使用一个或多个 *FMT_OPTS* 启用额外功能。

  指定 *BACKING_FILE* 后，只记录相对该文件的差异，此时无需指定大小。
  除非使用监视器的 ``commit`` 或 ``qemu-img commit``，否则不会修改后备文件。

  相对路径以 *FILE* 所在目录为基准。

  默认会打开后备文件并检查有效性。
  ``-u`` 启用不安全后备文件模式，即使无法打开后备文件，也创建镜像。
  真正使用这种镜像前，必须创建匹配的后备文件，或通过额外选项使后备文件配置有效。

  大小也可以通过 ``-o`` 中的 *SIZE* 选项指定，此时无需单独提供。

.. option:: dd [--image-opts] [-U] [-f FMT] [-O OUTPUT_FMT] [bs=BLOCK_SIZE] [count=BLOCKS] [skip=BLOCKS] if=INPUT of=OUTPUT

  从 *INPUT* 复制到 *OUTPUT*，同时由 *FMT* 转为 *OUTPUT_FMT*。

  默认以 512 字节块读写，可通过 *BLOCK_SIZE* 修改。
  如果指定 count=\ *BLOCKS*，读取该数量的输入块后停止。

  大小语法与 :manpage:`dd(1)` 类似。

.. option:: info [--object OBJECTDEF] [--image-opts] [-f FMT] [--output=OFMT] [--backing-chain] [--limits] [-t CACHE] [-U] FILENAME

  显示镜像 *FILENAME* 的信息，尤其可查看其实际占用的磁盘空间，
  这可能不同于显示的虚拟大小。镜像包含的虚拟机快照也会列出。

  若存在后备链，可用 ``--backing-chain`` 递归枚举各层镜像信息。

  例如，镜像链如下：

  ::

    base.qcow2 <- snap1.qcow2 <- snap2.qcow2


  从顶层到底层依次枚举：

  ::

    qemu-img info --backing-chain snap2.qcow2


  输出格式 *OFMT* 为 ``human`` 或 ``json``。
  JSON 默认是 QAPI ``ImageInfo`` 对象；使用 ``--backing-chain`` 时为对象数组。

  ``--output=human`` 为每个镜像报告以下信息：

  *image*
    镜像文件名。

  *file format*
    镜像格式。

  *virtual size*
    客户机磁盘大小。

  *disk size*
    镜像在宿主文件系统上的占用；如果无法获取，例如没有文件系统，可能显示为 0。

  *cluster_size*
    镜像格式的簇大小，如适用。

  *encrypted*
    是否加密，仅在加密时显示。

  *cleanly shut down*
    如果镜像为脏状态，下次在 QEMU 中打开需要自动修复，则显示 ``no``。

  *backing file*
    后备文件名称，如有。

  *backing file format*
    镜像强制指定的后备文件格式，如有。

  *Snapshot list*
    全部内部快照。

  *Format specific information*
    由格式决定的补充信息，是相应 ``ImageInfoSpecific*`` QAPI 对象的文本表示，
    例如 qcow2 对应 ``ImageInfoSpecificQCow2``。

  *Block limits*
    QEMU 检测到的镜像块 I/O 限制，仅指定 ``--limits`` 时显示。

.. option:: map [--object OBJECTDEF] [--image-opts] [-f FMT] [--start-offset=OFFSET] [--max-length=LEN] [--output=OFMT] [-U] FILENAME

  输出 *FILENAME* 及其后备链的元数据，特别是各扇区的分配状态，
  以及链中最靠上、为该扇区分配空间的文件。

  支持两种输出格式。默认 ``human`` 仅显示已知非零区域，
  省略已知全零区域及整条链中都未分配的区域。
  输出指出可读取数据的文件和偏移，每行四个字段，前三个为十六进制。
  例如：

  ::

    Offset          Length          Mapped to       File
    0               0x20000         0x50000         /tmp/overlay.qcow2
    0x100000        0x10000         0x95380000      /tmp/backing.qcow2


  第一行表示镜像从偏移 0 开始的 0x20000（131072）字节，
  位于按 ``raw`` 打开的 /tmp/overlay.qcow2 中，文件偏移为 0x50000（327680）。
  如果数据被压缩、加密或因其他原因无法按 raw 访问，human 格式会报错。
  文件名可能包含换行，因此脚本解析此输出不安全。

  ``json`` 格式返回字典数组，``start``、``length`` 和 ``offset``
  提供类似信息，此外还有：

  - 布尔字段 ``data``：扇区包含实际数据时为 true；未分配或以优化的全零簇保存时为 false。
  - 布尔字段 ``zero``：已知读取结果为零时为 true。
  - 布尔字段 ``present``：数据属于当前后备链时为 true；
    如果将链变基到更深文件后会从该文件取得数据，则为 false。
  - 整数字段 ``depth``：数据所在的链深度。例如 2 表示 *FILENAME* 的后备文件的后备文件。

  JSON 的 ``offset`` 是可选字段。
  human 格式会省略条目或报错的情况，不提供该字段。
  如果 ``data`` 为 false 但有 ``offset``，说明对应扇区尚未使用，但已预分配。

  更多信息见 QEMU 源码 ``include/block/block.h``。

.. option:: measure [--output=OFMT] [-O OUTPUT_FMT] [-o OPTIONS] [--size N | [--object OBJECTDEF] [--image-opts] [-f FMT] [-l SNAPSHOT_PARAM] FILENAME]

  计算新镜像所需的文件大小，可用于为逻辑卷或 SAN LUN 分配合适容量。
  报告值保证足以容纳镜像。
  输出格式 *OFMT* 为 ``human`` 或 ``json``，JSON 为 QAPI ``BlockMeasureInfo`` 对象。

  给出大小 *N* 时，按 ``qemu-img create`` 创建新空镜像估算。
  给出 *FILENAME* 时，按 ``qemu-img convert`` 转换现有镜像估算。
  新文件格式由 *OUTPUT_FMT* 指定，现有文件格式由 *FMT* 指定。

  可使用 *SNAPSHOT_PARAM* 指定现有镜像的内部快照。

  报告以下字段：

  ::

    required size: 524288
    fully allocated size: 1074069504
    bitmaps size: 0


  ``required size`` 是新镜像的文件大小；支持紧凑表示的格式可能小于虚拟磁盘大小。

  ``fully allocated size`` 是全部扇区写入数据后的文件大小。
  不考虑内部快照、脏位图、vmstate 及其他高级格式特性时，它是镜像的最大占用。

  ``bitmaps size`` 是除客户机可见数据外，复制源位图所需的额外空间。
  源或目标不支持位图时省略；支持但没有位图需要复制时为 0。

.. option:: snapshot [--object OBJECTDEF] [-f FMT | --image-opts] [-U] [-q] [-l | -a SNAPSHOT | -c SNAPSHOT | -d SNAPSHOT] FILENAME

  列出、应用、创建或删除 *FILENAME* 中的快照。

.. option:: rebase [--object OBJECTDEF] [--image-opts] [-U] [-q] [-f FMT] [-t CACHE] [-T SRC_CACHE] [-p] [-u] [-c] -b BACKING_FILE [-B BACKING_FMT] FILENAME

  改变镜像的后备文件，仅 ``qcow2`` 和 ``qed`` 支持。

  后备文件改为 *BACKING_FILE*；若 *FILENAME* 的格式支持，
  后备文件格式也改为 *BACKING_FMT*。
  *BACKING_FILE* 为空字符串时，移除后备关系，使镜像独立存在。

  相对路径以 *FILENAME* 所在目录为基准。

  *CACHE* 指定 *FILENAME* 的缓存模式，*SRC_CACHE* 指定读取后备文件的缓存模式。

  rebase 有两种模式：

  安全模式
    默认模式，会执行实际的数据变基。
    新旧后备文件可以不同，``qemu-img rebase`` 会确保客户机可见内容不变。

    为此，在改变后备文件之前，会先将新旧后备文件之间不同的簇合并到 *FILENAME*。
    指定 ``-c`` 时，只压缩合并写入的簇，而非整个镜像。

    此操作开销较大，类似镜像转换，且要求旧后备文件仍然存在。

  不安全模式
    指定 ``-u`` 时，仅更改后备文件名称及格式，不检查内容。
    用户必须提供正确的新后备文件，否则客户机可见内容会损坏。

    适用于重命名或移动后备文件，不要求旧后备文件仍可访问，
    因而可以修复后备文件已经移动或改名的镜像。

  rebase 也可以对两个镜像求差异。
  例如，复制或克隆客户机后，希望恢复为基于模板或底层镜像的精简差异镜像。

  假设复制 ``base.img`` 得到 ``modified.img``，后者已运行并产生修改。
  若要创建仅包含差异的 ``diff.qcow2``，执行：

  ::

    qemu-img create -f qcow2 -b modified.img diff.qcow2
    qemu-img rebase -b base.img diff.qcow2


  之后可以丢弃 ``modified.img``，因为 ``base.img + diff.qcow2`` 包含相同信息。

.. option:: resize [--object OBJECTDEF] [--image-opts] [-f FMT] [--preallocation=PREALLOC] [-q] [--shrink] FILENAME [+ | -]SIZE

  改变镜像大小，使其等效于创建时使用 *SIZE*。

  缩小镜像之前，必须先在虚拟机内使用文件系统和分区工具，
  相应缩小已分配的文件系统和分区，否则会丢失数据。

  缩小时必须提供 ``--shrink``，表示接受截断末尾之后全部数据的丢失。

  扩大镜像后，必须在虚拟机内用文件系统和分区工具扩容，才能实际使用新增空间。

  扩大时可用 ``--preallocation`` 指定宿主如何分配新增区域，
  有效值见 :ref:`notes` 中的格式说明。
  这可能比实际需要略多分配一些空间。

.. _notes:

说明与注意事项
--------------

支持的镜像格式如下。

``raw``

  原始磁盘格式，也是默认格式。结构简单，便于导出到其他模拟器。
  如果文件系统支持空洞，例如 Linux 的 ext2/ext3 或 Windows 的 NTFS，
  则只有写入过的扇区占用空间。
  用 ``qemu-img info``，或 Unix/Linux 的 ``ls -ls`` 查看实际占用。

  支持以下选项：

  ``preallocation``
    预分配模式，可选 ``off``、``falloc`` 和 ``full``。
    falloc 调用 ``posix_fallocate()`` 预分配；full 向底层存储写入数据分配空间。
    根据存储位置，这些数据可能为零，也可能不是。

``qcow2``

  QEMU 镜像格式，功能最全面。可缩小镜像，尤其适用于不支持空洞的文件系统，
  例如部分 Windows 环境；支持可选 AES 加密、zlib 或 zstd 压缩，以及多个虚拟机快照。

  支持以下选项：

  ``compat``
    决定 qcow2 版本。``compat=0.10`` 使用 QEMU 0.10 起都能读取的传统格式。
    默认 ``compat=1.1`` 启用只有 QEMU 1.1 及以后支持的扩展，
    包括有助于对稀疏镜像高效执行读时复制的零簇等。

  ``backing_file``
    底层镜像文件名，见 create 子命令。

  ``backing_fmt``
    底层镜像格式。

  ``compression_type``
    配置压缩簇使用的算法，但仅设置此选项并不会自动启用压缩写入。
    通常与 ``qemu-img convert -c`` 一起使用，
    也可以配合 compress 过滤驱动或启用了压缩的备份块作业。

    有效值为 ``zlib`` 和 ``zstd``；``compat=0.10`` 仅支持 zlib。

  ``encryption``
    设为 ``on`` 时，使用 128 位 AES-CBC 加密。

    按现代密码学标准，qcow 和 qcow2 的这种加密方式存在多个设计问题：

    - AES-CBC 的初始化向量基于扇区号，可预测，
      容易受到能揭示加密数据存在性的选择明文攻击。

    - 用户口令直接作为密钥，短口令或选择不当会破坏加密安全性。

    - 口令泄露后，无法仅更改口令保护已有 qcow 镜像。
      必须克隆到采用另一口令的新文件，再用 shred 等程序安全擦除原文件，
      但这种擦除对许多现代存储技术也不可靠。

    - 扇区初始化向量基于客户机虚拟扇区号，而非宿主物理扇区号。
      多个内部快照因此可能让不同物理扇区使用相同 IV。
      如果攻击者收集到这些扇区及部分可预测数据，CBC 模式可能遭受水印攻击。
      多个 qcow2 镜像使用相同口令也存在此问题，因为口令直接用作密钥。

    因此强烈不建议使用 qcow/qcow2 的这种加密方式，
    推荐 Linux dm-crypt/LUKS 等替代技术。

  ``cluster_size``
    设置簇大小，必须在 512 字节到 2M 之间。
    小簇有助于缩小文件，大簇通常性能更好。

  ``preallocation``
    可选 ``off``、``metadata``、``falloc`` 和 ``full``。
    预分配元数据使初始镜像较大，但增长时性能更好。
    falloc 和 full 类似 raw 的同名选项，同时还会建立元数据。

  ``lazy_refcounts``
    设为 ``on`` 时延后引用计数更新，以减少元数据 I/O、提高性能。
    对不会批量更新元数据的 ``cache=writethrough`` 尤其有用。
    代价是宿主崩溃后必须重建引用计数表，
    即下次打开时自动运行 ``qemu-img check -r all``，可能耗费一些时间。

    只有 ``compat=1.1`` 时才能启用。

  ``nocow``
    设为 ``on`` 时关闭文件的写时复制，仅对 btrfs 有效。

    btrfs 承载虚拟机镜像的性能较低，客户机也使用 btrfs 时尤甚。
    关闭 COW 可以缓解，通常有两种方式：

    - 使用 nodatacow 挂载，使全部新文件具有 NOCOW 属性。
    - 为空文件添加 NOCOW 属性，本选项采用这种方式。

    此选项仅适用于新文件或空文件。已有数据块的 COW 文件无法仅通过 nocow=on
    转为 NOCOW。可用 ``lsattr filename`` 查看属性，大写 C 表示 NOCOW。

  ``data_file``
    保存全部客户机数据的文件名。使用后，qcow2 文件只包含镜像元数据。

    注意：如果在 qemu-img create 中指定已存在的文件，会重新创建数据文件，
    覆盖原内容并造成数据丢失。
    如果只想让引用指向已有文件，请使用 ``qemu-img amend``。

  ``data_file_raw``
    设为 ``on`` 时，QEMU 始终让外部数据文件保持为可独立读取的只读 raw 镜像。

    对 qcow2 的全部写入，包括偏移，会原样转发到 raw 文件。
    因此客户机在某偏移看到的数据，也位于 raw 文件的相同偏移。
    此 raw 镜像应只读使用：绕过 qcow2 元数据写入可能使元数据与 raw 内容失去同步，
    导致 qcow2 元数据损坏。

    设为 ``off`` 时，数据文件采用任意存储布局，
    没有配套 qcow2 元数据就无法理解其内容。
    实际写入位置与客户机偏移无关，部分写入，尤其零写入，
    可能只修改 qcow2 元数据，而完全不写数据文件。

    只有设置 ``data_file`` 后才能启用。

``其他``

  为兼容旧 QEMU 或其他虚拟机监控器，还支持 VMDK、VDI、VHD（vpc）、VHDX、
  qcow1 和 QED 等格式。完整列表见 ``qemu-img --help``，
  详细说明见 QEMU 块驱动参考文档。

  这些格式的块驱动主要用于镜像转换。
  运行虚拟机时，建议先转换为 raw 或 qcow2，以获得良好性能。
