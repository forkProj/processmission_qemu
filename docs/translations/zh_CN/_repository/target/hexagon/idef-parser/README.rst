将 Hexagon ISA 指令定义编译为 TCG 生成器
----------------------------------------

idef-parser 是一个小型编译器，将 Hexagon ISA 描述语言转换为便于集成到 QEMU
的 tinycode generator（TCG）代码。

编译示例
--------

下面以最简单的 Hexagon 指令之一 ``add`` 说明 idef-parser 的用途。

ISA 描述如下：

.. code:: c

   A2_add(RdV, in RsV, in RtV) {
       { RdV=RsV+RtV;}
   }


idef-parser 将其编译为：

.. code:: c

   /* A2_add */
   void emit_A2_add(DisasContext *ctx, Insn *insn, Packet *pkt, TCGv_i32 RdV,
                    TCGv_i32 RsV, TCGv_i32 RtV)
   /*  { RdV=RsV+RtV;} */
   {
       TCGv_i32 tmp_0 = tcg_temp_new_i32();
       tcg_gen_add_i32(tmp_0, RsV, RtV);
       tcg_gen_mov_i32(RdV, tmp_0);
   }


输出是一个函数，其中的 TCG 生成器实现对应指令语义。
函数不访问全局变量，所需数据结构都通过参数显式传入。
参数包括表示输入输出寄存器的 TCGv、表示指令立即数的整数，
以及保存反汇编上下文的 ``DisasContext`` 等结构。

输入中的 add 指令有唯一标识符 ``A2_add``，
用于区分同一指令的不同变体，并表达所属类别。
这里 A2 对应 Hexagon 的 ALU32/ALU 指令子类。

标识符后的参数表示将传给生成函数的 TCG 变量。
标有 in 的参数已经初始化，其余为输出参数。

编译器据此：

- 为输出函数签名填入正确的 TCGv 寄存器。
- 为函数签名填入立即数整数。
- 跟踪已声明寄存器中哪些已经初始化。

本例的实际指令描述代码为：

.. code:: c

   { RdV=RsV+RtV;}


它使用 C 语法的子集，由 ``macros.h`` 中的部分宏展开而来。
这些宏把相似结构的复杂变体映射到统一原语，
简化输入语言，减少解析器需要处理的计算原语数量。

描述代码操作声明语句定义的寄存器。
本例三个寄存器都会声明，RsV 和 RtV 被读取，RdV 被写入。

逐行查看生成代码：

::

   TCGv_i32 tmp_0 = tcg_temp_new_i32();


先声明临时 TCGv，保存加法结果。

::

   tcg_gen_add_i32(tmp_0, RsV, RtV);


再生成对选定寄存器求和的 tinycode 操作，将结果保存到临时变量。

::

   tcg_gen_mov_i32(RdV, tmp_0);


最后移入目标寄存器。虽然看起来不够高效，
QEMU 后续会优化 tinycode，消除不必要的复制。

解析器输入
----------

介绍实现结构前，先说明输入。
``semantics_generated.pyinc`` 中生成的指令语义会经过两个预处理步骤。
首先：

::

    gen_idef_parser_funcs.py


该步骤将语义转换为类 C 伪代码，写入 ``idef_parser_input.h.inc``。
例如，跳转到寄存器参数地址的 J2_jumpr，在原文件中定义为：

::

    SEMANTICS( \
        "J2_jumpr", \
        "jumpr Rs32", \
        """{fJUMPR(RsN,RsV,COF_TYPE_JUMPR);}""" \
    )


运行 gen_idef_parser_funcs.py 后得到：

::

    J2_jumpr(in RsV) {
        {fJUMPR(RsN,RsV,COF_TYPE_JUMPR);}
    }


其中 fJUMPR 等宏仍未展开。

第二步将宏展开为适合解析器的形式。
这些宏定义在 ``idef-parser/macros.h.inc``。
prepare 脚本调用 C 预处理器处理 ``idef_parser_input.h.inc``，
生成 ``idef_parser_input.preprocessed.h.inc``。

J2_jumpr 预处理后为：

::

    J2_jumpr(in RsV) {
        {(PC = RsV);}
    }


``fJUMPR(RsN,RsV,COF_TYPE_JUMPR);`` 展开成 ``(PC = RsV)``，表示写入程序计数器。
PC 并非严格意义上的 C 变量，因为它没有声明，
而是方便 idef-parser 后续匹配的符号。

解析器结构
----------

idef-parser 基于 flex 和 bison。

flex 按正则表达式将输入分成词法单元，定义位于 ``idef-parser.lex``。
扫描器还提取其他有意义的信息，例如立即数的数值，并将其附加到词法单元。

bison 根据 ``idef-parser.y`` 的语法描述生成解析器。
程序执行该文件末尾的 main，打开输入输出文件、创建上下文，
再调用 ``yyparse()`` 启动 LALR(1) 解析，详见
`LALR 解析说明 <https://en.wikipedia.org/wiki/LALR_parser>`__。
每次移入词法单元时，会调用 flex 生成的 ``yylex()``，
从输入文件读取并返回下一个单元。

词法单元按 idef-parser.y 中的文法，结合运算符优先级和结合规则，构建唯一语法树。

文法描述包含全部 Hexagon 指令的文件，因此从表示指令列表的 input 非终结符开始。
单条指令使用以下规则，对应前述输入结构：

::

   instruction : INAME arguments code
               | error

   arguments : '(' ')'
             | '(' argument_list ')';

   argument_list : argument_decl ',' argument_list
                 | argument_decl

   argument_decl : REG
                 | PRED
                 | IN REG
                 | IN PRED
                 | IMM
                 | var
                 ;

   code        : '{' statements '}'

   statements  : statements statement
               | statement

   statement   : control_statement
               | var_decl ';'
               | rvalue ';'
               | code_block
               | ';'

   code_block  : '{' statements '}'
               | '{' '}'


这一部分定义指令、参数及语句。参数由 argument_decl 定义，可以是：

::

    Description                  Example
    ----------------------------------------
    output register              RsV
    output predicate register    P0
    input register               in RsV
    input predicate register     in P0
    immediate value              1234
    local variable               EA


唯一允许作为参数的局部变量是有效地址 EA。
语句可以是 control_statement、``int a;`` 这样的变量声明、
花括号包围的语句块、表示空操作的分号，或 ``rvalue ';'``。

表达式
~~~~~~~~

输入支持 C 表达式，但为简化解析有一些例外。
RdV、RssV、PdV、CsV 等 Hexagon 惯用寄存器名称专供寄存器参数使用，
根据大小映射到 TCGv_i32 或 TCGv_i64。
UiV、riV 等立即数参数则映射到 C 整数。

PC、SP、FP 等名称表示程序计数器、栈指针、帧指针等寄存器。
写入对应 ``PC = ...`` 一类赋值，读取则对应使用 PC 变量。

另一个例外是选择性展开 macros.h 中的宏。例如，fABS 在普通 C 中定义为：

::

    #define fABS(A) (((A) < 0) ? (-(A)) : (A))


它返回 A 的绝对值。由于不包含在 idef-parser/macros.h.inc 中，
不会展开，而保留为 ``fABS(...)`` 调用形式。
相比完整三元表达式，这更容易匹配并映射到 ``tcg_gen_abs_<width>``。
许多宏都因此保留不展开，详见 ``idef-parser/idef-parser.lex``。

映射到 TCG 生成器时，idef-parser 尽量使用普通 C 完成计算，
例如直接在 C 中执行二元运算而非生成 tinycode，实际实现常量折叠。

变量及变量声明
~~~~~~~~~~~~~~

与 C 类似，变量必须显式声明，例如 ``int var1;`` 声明未初始化变量，
也支持预期的 ``int var2 = 0;`` 初始化。
它们映射到：

::

    int var1;           ->      TCGv_i32 var1 = tcg_temp_new_i32();

    int var2 = 0;       ->      TCGv_i32 var1 = tcg_temp_new_i32();
                                tcg_gen_movi_i32(j, ((int64_t) 0ULL));


这些变量在所在函数结束时自动释放。
与 C 不同，仅允许下表中的整数类型，且不能调整关键字顺序：

::

    type                        bit-width    signedness
    ----------------------------------------------------------
    int                         32           signed
    signed
    signed int

    unsigned                    32           unsigned
    unsigned int

    long                        64           signed
    long int
    signed long
    signed long int

    unsigned long               64           unsigned
    unsigned long int

    long long                   64           signed
    long long int
    signed long long
    signed long long int

    unsigned long long          64           unsigned
    unsigned long long int

    size[1,2,4,8][s,u]_t        8-64         signed or unsigned


变量名匹配通用 VARID 词法单元，其附加数据保存名称。
声明时调用 gen_varid_allocate，记录名称、大小和位宽，
同时检查重复声明并在发现时报告错误。
使用变量时，通过 VARID 查找其大小和位宽。

类型系统
~~~~~~~~

idef-parser 的简单类型系统用于正确实现运算的符号性和位宽。

每个 rvalue 的类型由 ``unsigned bit_width`` 和 ``HexSignedness signedness`` 决定。

操作数类型影响处理和生成方式。
例如，有符号右移采用算术移位，无符号右移采用逻辑移位。
两个操作数一个有符号、一个无符号时，运算按有符号处理。

位宽也影响结果。输入语言有 8、16、32、64 位等细粒度类型，
向量指令还可能更宽，而 tinycode 只有 32 和 64 位。
编译器尽量传播原始细粒度类型，直到值参与 rvalue 间的运算。
此时如果任一操作数超过 32 位，就将整个运算提升到 64 位，并正确扩展两个操作数。
关键指令通常已有显式类型转换和零扩展、符号扩展，可正确传递到解析器。

rvalue 组合由 gen_bin_op 和 gen_bin_cmp 处理，
二者在适当的编译期或运行期生成所需计算。

控制语句
~~~~~~~~

control_statement 根据输入参数改变生成代码的执行顺序，其文法为：

::

   control_statement : frame_check
                     | cancel_statement
                     | if_statement
                     | for_statement
                     | fpart1_statement


if_statement 需要生成标签和条件跳转 ``tcg_gen_brcondi``。
生成器并不完全遵循 C 的控制语义：不实现 && 短路，也禁止使用 ||。
所有带谓词的指令，以及一般的可选择 lvalue 赋值，例如 C 风格三元表达式：

::

   rvalue            : rvalue QMARK rvalue COLON rvalue


都使用条件移动 ``tcg_gen_movcond``，避免管理标签和跳转的额外复杂性。

for 循环总是对立即数迭代，范围在编译期已知，因此直接生成普通 C for 循环。
循环在 QEMU 代码中执行，重复调用 tinycode 生成器，
最终生成的 tinycode 就是展开后的循环。

解析上下文
~~~~~~~~~~

idef-parser.y 中的辅助函数都接受两个固定参数：解析上下文 c 和位置 YYLLOC。
上下文显式传递是因为解析器可重入，没有全局变量，方便未来并行编译指令。
每条规则还传播词法单元位置，用于生成清晰错误报告，并高亮出错输入片段。

调试
--------

开发时主要会遇到编译期错误和解析错误。

Bison 解析器的编译期错误通常来自文法冲突，导致无法生成唯一推导树，必须解决。
悬空 else 是例外，通过 ``%expect 1`` 标记为预期冲突。

解决冲突需要了解 `移入/归约冲突
<https://www.gnu.org/software/Bison/manual/html_node/Shift_002fReduce.html>`__ 和
`归约/归约冲突 <https://www.gnu.org/software/Bison/manual/html_node/Reduce_002fReduce.html>`__。
Bison 高于 3.7.1 时，可使用 -Wcex 生成展示歧义推导的反例。
移入/归约冲突通常通过改写无歧义文法或设置正确优先级解决，
归约/归约冲突则需要重新设计相关文法。

运行时错误分词法和语法错误。
词法错误不易发现，因为 var 会接收其他规则未匹配的内容；
但修复通常只需调整正则表达式。

idef-parser 的错误报告会显示触发错误规则的输入片段。

指令实现要经过以下阶段，排查建议如下：

-  not-emitted

   指令输入解析失败，可能是词法错误，或有效词法单元的顺序不符合任何规则。
   应检查识别、映射是否正确，以及是否存在匹配目标序列的规则。

-  emitted

   已生成指令，但将其加入 QEMU 后无法编译。
   构建错误会提供线索，常见问题是 TCG 生成器参数不匹配，
   根源通常在 idef-parser 类型系统。

-  compiled

   已生成可编译代码，但 QEMU 或 harness 测试失败。
   需要检查失败用例、生成器指令和实际 tinycode。
   建议先处理 harness 测试，因为通常只涉及单条指令，跟踪更容易。
   多线程测试失败时，可以先修其他测试，期待它随之被间接修复。

   下一节提供完整排查示例。

-  tests-passed

   最终目标：指令通过测试套件。

如果系统测试中许多指令失败，还可以将执行跟踪与 QEMU 已有参考实现比较。
先获得能通过测试的 QEMU，并运行：

::

   sudo unshare -p sudo -u <USER> bash -c \
   'env -i <qemu-hexagon full path> -d cpu <TEST>'


对自己的实现执行相同操作，跟踪会自然对齐，可用 diff 查找行为差异。

排查错误 TCG 生成器的示例
~~~~~~~~~~~~~~~~~~~~~~~~~

本节演示如何排查单条指令错误生成的 TCG 代码。

先在 A2_add 中引入错误：

::

    void emit_A2_add(DisasContext *ctx, Insn *insn, Packet *pkt, TCGv_i32 RdV,
                     TCGv_i32 RsV, TCGv_i32 RtV)
    /*  RdV=RsV+RtV;} */
    {
        TCGv_i32 tmp_0 = tcg_temp_new_i32();
        tcg_gen_add_i32(tmp_0, RsV, RsV);
        tcg_gen_mov_i32(RdV, tmp_0);
    }


错误位于 ``tcg_gen_add_i32(tmp_0, RsV, RsV);``，
计算了 RsV + RsV，而非预期的 RsV + RtV。
A2_add 极为常用，因此几乎所有测试都会失败，反而难以定位。

例如运行 check-tcg：

::

    make check-tcg TIMEOUT=1200 \
                   DOCKER_IMAGE=debian-hexagon-cross \
                   ENGINE=podman V=1 \
                   DOCKER_CROSS_CC_GUEST=hexagon-unknown-linux-musl-clang


首个 float_convs 用例就因段错误失败，harness 和 libc 测试也会全部失败。
此时需要逐步排除指令。可先用 ``-d in_asm,cpu`` 查看执行的 Hexagon 指令及 CPU 状态，
并准备一个正常模拟器作对照：

::

    meson configure -Dhexagon_idef_parser=false


这会对全部指令禁用 idef-parser，回退到手写生成器覆盖或辅助函数实现。
重新编译得到可通过全部测试的 qemu-hexagon。
假设错误版本名为 qemu-hexagon-buggy，比较两者：

::

    ./qemu-hexagon-buggy -d in_asm,cpu float_convs &> out_buggy
    ./qemu-hexagon       -d in_asm,cpu float_convs &> out_working


``diff -u out_buggy out_working`` 显示第 141 行开始状态分歧，R1 值错误：

::

    @@ -138,7 +138,7 @@

     General Purpose Registers = {
       r0 = 0x4100f9c0
    -  r1 = 0x00042108
    +  r1 = 0x00000000
       r2 = 0x00021084
       r3 = 0x00000000
       r4 = 0x00000000


直接查看 out_buggy，可在第 141 行附近看到相应输入汇编：

::

    116 |  ----------------
    117 |  IN: _start_c
    118 |  0x000210b0:  0xa09dc002	{	allocframe(R29,#0x10):raw }
    ... |  ...
    137 |  0x000210fc:  0x5a00c4aa	{	call PC+2388 }
    138 |
    139 |  General Purpose Registers = {
    140 |    r0 = 0x4100fa70
    141 |    r1 = 0x00042108
    142 |    r2 = 0x00021084
    143 |    r3 = 0x00000000


虽然 CPU 状态打印在汇编之后，但它实际是执行这些指令之前的状态，
因此需要检查更早执行的指令。

向上查看可见：

::

    54 |  ----------------
    55 |  IN: _start
    56 |  0x00021088:  0x6a09c002	{	R2 = C9/pc }
    57 |  0x0002108c:  0xbfe2ff82	{	R2 = add(R2,#0xfffffffc) }
    58 |  0x00021090:  0x9182c001	{	R1 = memw(R2+#0x0) }
    59 |  0x00021094:  0xf302c101	{	R1 = add(R2,R1) }
    60 |  0x00021098:  0x7800c01e	{	R30 = #0x0 }
    61 |  0x0002109c:  0x707dc000	{	R0 = R29 }
    62 |  0x000210a0:  0x763dfe1d	{	R29 = and(R29,#0xfffffff0) }
    63 |  0x000210a4:  0xa79dfdfe	{	memw(R29+#0xfffffff8) = R29 }
    64 |  0x000210a8:  0xbffdff1d	{	R29 = add(R29,#0xfffffff8) }
    65 |  0x000210ac:  0x5a00c002	{	call PC+4 }
    66 |
    67 |  General Purpose Registers = {
    68 |    r0 = 0x00000000
    69 |    r1 = 0x00000000
    70 |    r2 = 0x00000000
    71 |    r3 = 0x00000000


第 56–65 行的指令将在下方所列 CPU 状态上执行。
此时尚未分歧，说明起始状态正确，错误必在这些指令中。
R1 只被第 57、58 行修改：

::

    58 |  0x00021090:  0x9182c001	{	R1 = memw(R2+#0x0) }
    59 |  0x00021094:  0xf302c101	{	R1 = add(R2,R1) }


因此问题要么在加载 ``R1 = memw(R2+#0x0)``，要么在加法 ``R1 = add(R2,R1)``。
可以直接检查相应生成代码，也可以运行 ``./qemu-heaxgon -d op,in_asm float_conv``，
查看 add 对应的 tinycode：

::

   ---- 00021094
   mov_i32 pkt_has_scalar_store_s1,$0x0
   add_i32 tmp0,r2,r2
   mov_i32 loc2,tmp0
   mov_i32 new_r1,loc2
   mov_i32 r1,new_r1


由此定位到错误 ``add_i32 tmp0,r2,r2``。

限制与未来改进
--------------

当前解析器主要受 Bison 语法驱动机制限制：只能按规则求值顺序生成代码，
无法回溯修改已经生成的内容。

例如输入语句：

::

   { (PsV==0xff) ? (PdV=0xff) : (PdV=0x00); }


这种三元赋值需要生成跳转到第一或第二代码块的控制流。
但匹配到三元赋值时，两侧赋值代码早已生成，因而实现非常复杂。

因此先预处理为：

::

   { PdV = ((PsV==0xff)) ? 0xff : 0x00; }


便可由以下规则轻松匹配：

::

   statement             | rvalue ';'

   rvalue                : rvalue QMARK rvalue COLON rvalue
                         | rvalue EQ rvalue
                         | LPAR rvalue RPAR
                         | assign_statement
                         | IMM

   assign_statement      : pred ASSIGN rvalue


前面的加法也体现了此限制：

::

   TCGv_i32 tmp_0 = tcg_temp_new_i32();
   tcg_gen_add_i32(tmp_0, RsV, RtV);
   tcg_gen_mov_i32(RdV, tmp_0);


无法直接将 RdV 用作加法目标，是因为匹配赋值时，
代表加法的 rvalue 已经归约，对应代码也已生成，不能修改。
我们依赖 QEMU 后续优化，消除无用移动及临时变量。

未来可以支持向量和浮点指令，需要扩展扫描器、解析器，并部分重设类型系统，
基于已有向量 TCG 原语实现向量语义。

更彻底的方案是先生成 LLVM IR 等中间表示，再由 clang TCG 后端编译，
而非直接生成 TCG 生成器。
这样可以进一步优化，克服语法驱动解析的限制，改善最终生成代码。
