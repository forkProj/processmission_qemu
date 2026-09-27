.. _decodetree:

===============
Decodetree 规格
===============

decodetree 由指令模式组成。根据后续处理需要，
一个模式可以表示一条架构指令，也可以表示一组指令。

每个模式都有 fixedbits 和 fixedmask，两者共同定义匹配条件::

  (insn & fixedmask) == fixedbits


模式还可定义字段，从 insn 提取后传给翻译器，例如寄存器、立即数和子操作码。

可以声明并复用字段、参数集和格式，简化后续模式定义。

字段
========

语法::

  field_def     := '%' identifier ( field )* ( !function=identifier )?
  field         := unnamed_field | named_field
  unnamed_field := number ':' ( 's' ) number
  named_field   := identifier ':' ( 's' ) number


unnamed_field 的第一个数字表示最低位位置，第二个数字表示字段长度。
存在 s 时按有符号字段处理。

named_field 引用指令模式或格式中的其他字段。
无论被引用字段原本多长，都会按指定符号属性和位宽插入当前字段。

字段直接或间接引用自身而形成循环，是错误。

格式可包含引用使用该格式的指令模式中命名字段的字段；
反过来，指令模式也可引用其格式中定义的命名字段。
但目前不能同时使用两种方向：例如 P 使用 F，F 的 A 引用 P 的 B，
同时 P 的 C 又引用 F 的 D。

存在多个 fields 时，将它们拼接，由此可以描述分散字段。

指定 !function 时，将拼接结果交给该函数处理，输入和输出均为整数。

也可在没有 fields 时使用 !function，称为参数。
此时函数只接收 DisasContext，并返回从中提取的整数。

既没有 fields 也没有 !function 的定义无效。

字段示例：

+-----------------------------+----------------------------------------------+
| 输入                        | 生成代码                                     |
+=============================+==============================================+
| ``%disp   0:s16``           | ::                                           |
|                             |                                              |
|                             |   sextract(i, 0, 16)                         |
+-----------------------------+----------------------------------------------+
| ``%imm9   16:6 10:3``       | ::                                           |
|                             |                                              |
|                             |   extract(i, 16, 6) << 3 | extract(i, 10, 3) |
+-----------------------------+----------------------------------------------+
| ``%disp12 0:s1 1:1 2:10``   | ::                                           |
|                             |                                              |
|                             |   sextract(i, 0, 1) << 11 |                  |
|                             |       extract(i, 1, 1) << 10 |               |
|                             |       extract(i, 2, 10)                      |
+-----------------------------+----------------------------------------------+
| ``%shimm8 5:s8 13:1         | ::                                           |
| !function=expand_shimm8``   |                                              |
|                             |   expand_shimm8(sextract(i, 5, 8) << 1 |     |
|                             |                 extract(i, 13, 1))           |
+-----------------------------+----------------------------------------------+
| ``%sz_imm 10:2 sz:3         | ::                                           |
| !function=expand_sz_imm``   |                                              |
|                             |   expand_sz_imm(extract(i, 10, 2) << 3 |     |
|                             |                 extract(a->sz, 0, 3))        |
+-----------------------------+----------------------------------------------+

参数集
========

语法::

  args_def    := '&' identifier ( args_elt )+ ( !extern )?
  args_elt    := identifier (':' identifier)?


每个 args_elt 定义一个参数。
包含冒号时，前一个标识符是参数名，后一个是类型；没有冒号则类型为 int。

参数集生成为名为 arg_$name 的 C 结构，每个参数对应一个成员字段。

指定 !extern 时，假定底层结构已声明，通常由另一个解码器提供。

参数集适合为翻译函数编写处理公共参数的辅助函数。
例如让 AND 和 OR 模式把操作数放进同一命名结构，
从而通过共同的 gen_logic_insn 处理两者共享的操作。

参数集示例::

  &reg3       ra rb rc
  &loadstore  reg base offset
  &longldst   reg base offset:int64_t



格式
========

语法::

  fmt_def      := '@' identifier ( fmt_elt )+
  fmt_elt      := fixedbit_elt | field_elt | field_ref | args_ref
  fixedbit_elt := [01.-]+
  field_elt    := identifier ':' 's'? number
  field_ref    := '%' identifier | identifier '=' '%' identifier
  args_ref     := '&' identifier


格式可避免在大量指令模式中重复同一组字段。

fixedbit_elt 描述连续的固定 1、固定 0 或无关位。
点号表示稍后由字段或模式中的最终 0/1 覆盖，
连字符则表示 CPU 确实忽略该位，不再指定。

field_elt 描述仅指定宽度的简单字段，
位置由它与其他 fixedbit_elt、field_elt 的相对顺序推导。

只要出现任一 fixedbit_elt 或 field_elt，就必须定义全部位。
可以用全为点号的 fixedbit_elt 填充。

field_ref 通过引用引入字段，这是向格式添加复杂字段的唯一方式。
通过赋值给其他标识符，还可同时重命名，
让同一参数集适用于名称不同的分散字段。

一个 args_ref 可以指定格式所用参数集。
格式字段必须是参数集的子集；未指定时，根据字段集合推导参数集。

建议把 field_ref 和 args_ref 都放在行尾，
不要与 fixedbit_elf 或 field_elt 交错，但这不是强制要求。

格式示例::

  @opr    ...... ra:5 rb:5 ... 0 ....... rc:5
  @opi    ...... ra:5 lit:8    1 ....... rc:5


模式
========

语法::

  pat_def      := identifier ( pat_elt )+
  pat_elt      := fixedbit_elt | field_elt | field_ref | args_ref | fmt_ref | const_elt
  fmt_ref      := '@' identifier
  const_elt    := identifier '=' number


fixedbit_elt 和 field_elt 与格式中的含义相同。
未指定命名格式时，根据引用的参数集（如果存在）及字段集合推导格式。

const_elt 可将参数设为常量。
当不同模式的字段重叠、必须改在 fixedbit_elt 中指定值时，这很有用。

每匹配一个模式，解码器都会调用相应翻译函数。

模式示例::

  addl_r   010000 ..... ..... .... 0000000 ..... @opr
  addl_i   010000 ..... ..... .... 0000000 ..... @opi


其中会调用::

  trans_addl_r(ctx, &arg_opr, insn)


以及::

  trans_addl_i(ctx, &arg_opi, insn)


模式组
========

语法::

  group            := overlap_group | no_overlap_group
  overlap_group    := '{' ( pat_def | group )+ '}'
  no_overlap_group := '[' ( pat_def | group )+ ']'


组以单独一行的左花括号或左方括号开始，后续行缩进两格，
以单独一行的对应右括号结束。
组可以嵌套，每层增加两格缩进。

重叠组内的模式允许相互重叠，按顺序选择来解决冲突。
某模式的全部 fixedbits 匹配时，调用其翻译函数。
函数返回 false 后，继续匹配组中后续模式。

非重叠组与未分组模式一样，不允许重叠。
因此，这类组主要用于嵌套在重叠组内部。

以下 PA-RISC 示例展示 or 指令的特殊化处理::

  {
    {
      nop   000010 ----- ----- 0000 001001 0 00000
      copy  000010 00000 r1:5  0000 001001 0 rt:5
    }
    or      000010 rt2:5 r1:5  cf:4 001001 0 rt:5
  }


cf 为零时，指令无副作用，可以特殊处理。
rt 为零时结果被丢弃，指令没有效果。
rt2 为零时操作为 reg[r1] | 0，即标准寄存器复制。

生成器输出可能如下::

  switch (insn & 0xfc000fe0) {
  case 0x08000240:
    /* 000010.. ........ ....0010 010..... */
    if ((insn & 0x0000f000) == 0x00000000) {
        /* 000010.. ........ 00000010 010..... */
        if ((insn & 0x0000001f) == 0x00000000) {
            /* 000010.. ........ 00000010 01000000 */
            extract_decode_Fmt_0(&u.f_decode0, insn);
            if (trans_nop(ctx, &u.f_decode0)) return true;
        }
        if ((insn & 0x03e00000) == 0x00000000) {
            /* 00001000 000..... 00000010 010..... */
            extract_decode_Fmt_1(&u.f_decode1, insn);
            if (trans_copy(ctx, &u.f_decode1)) return true;
        }
    }
    extract_decode_Fmt_2(&u.f_decode2, insn);
    if (trans_or(ctx, &u.f_decode2)) return true;
    return false;
  }
