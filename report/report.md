# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 3: 断，都可以断（中断与异常） |
| **小组成员** | 2412796-杨斯源（组长）、2413628-黄乐仁、2410868-孙锘梵 |
| **完成日期** | [YYYY-MM-DD] |

### 小组分工

练习如何分工？

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2412796-杨斯源（组长） | 部分三：收口与验证（报告汇总 + 全量验证 + 格式校对） |
| 2413628-黄乐仁 | 部分一：Trap 上下文恢复与 Trap 入口（`RESTORE_ALL` + `__alltraps`）+ 补回 lab2 的 `page_init()` |
| 2410868-孙锘梵 | 部分二：中断与异常处理（`interrupt_handler` + `exception_handler`）+ 补回 lab2 的 Best-Fit |

实验报告如何分工？

报告的撰写按上表划分：各功能模块由其负责人撰写对应章节；实验环境、总结与收获及 `prompt.md` 的汇总由部分三负责人负责；最后经统一校对格式与图片引用后提交。

---

## 一、实验目的

本实验的主要目的是：

1. **理解 RISC-V 的中断与异常机制**：区分「中断（异步、外部事件）」与「异常（同步、指令执行引起）」，理解 M/S/U 特权级、`scause`/`sepc`/`stvec`/`sstatus` 等 CSR 的作用，以及 OpenSBI 如何把 S 模式下的 Trap 委托给内核处理。
2. **掌握 Trap 上下文的保存与恢复**：理解 `struct trapframe` 的布局与 `SAVE_ALL` 的保存关系，据此实现对应的 `RESTORE_ALL`，并让 `__alltraps` 按 RISC-V 调用约定把 Trap frame 交给 C 语言处理函数。
3. **掌握中断与异常处理程序的编写**：实现时钟中断的「设置下一次事件 → 计数 → 输出统计」，以及非法指令、断点异常的识别与 `sepc` 修正，理解为什么异常返回前必须正确更新 `epc`。
4. **理解中断的关闭与开启及原子操作**：理解为什么部分操作需要关中断保护，掌握 `local_intr_save/restore` 与 `atomic.h` 提供的位操作。

---

## 二、实验环境

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| 2412796-杨斯源 | [待本人确认] | [待本人确认] | |
| 2413628-黄乐仁 | [待本人确认] | [待本人确认] | |
| 2410868-孙锘梵 |DeepSeek Harness|deepseek-flash| |

**运行环境：**

| 组件 | 版本 / 包名 |
|------|-------------|
| 操作系统 | Ubuntu 24.04.5 LTS（WSL2） |
| 交叉编译器 | `riscv64-unknown-elf-gcc` 15.1.0 |
| 二进制工具 | `riscv64-unknown-elf-ld` / `objcopy` / `objdump` |
| 模拟器 | `qemu-system-riscv64` 8.2.2（apt 包 `qemu-system-misc`，自带 OpenSBI v1.3） |
| 调试器 | `riscv64-unknown-elf-gdb` 16.3.90 |

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

<!-- 由部分三负责人起稿，全员过目
     建议围绕：本章在 lab2 的分页内核之上，加入「对硬件事件的响应能力」。
     核心链路：stvec 指向 __alltraps → 发生 Trap → SAVE_ALL 保存现场 →
     调用 trap(tf) → trap_dispatch 按 cause 分派 → interrupt_handler / exception_handler →
     返回 __trapret → RESTORE_ALL 恢复现场 → sret 回到原执行流 -->

[待填写]

### 3.2 功能的逐步实现

<!-- 按"为什么是这个顺序"来讲，建议顺序：
     1. 前置：补回 lab2 的 page_init 与 Best-Fit（为什么：本实验依赖 lab2 的内存管理）
     2. 设置 Trap 入口：idt_init 写 stvec = __alltraps（为什么：让硬件知道 Trap 往哪跳）
     3. 保存现场：SAVE_ALL（已提供）、构造 trapframe（为什么：C 函数需要完整上下文）
     4. 调用 C 处理器：__alltraps 里按调用约定把 tf 传给 trap()（为什么：汇编到 C 的桥）
     5. 分派与处理：trap_dispatch 按 cause 符号区分中断/异常，交给两个 handler（为什么：不同 Trap 处理方式不同）
     6. 恢复现场并返回：RESTORE_ALL + sret（为什么：要回到被打断的地方继续执行）
     7. 时钟中断闭环：clock_init 设置首次事件 → 每次时钟中断重新 arm → ticks 累加（为什么：一次性定时器必须每次都重设） -->

[待填写]

---

## 四、实验内容与实现

<!--
说明：按练习文档中的功能模块顺序组织。
每个模块下需写清：涉及函数、功能说明、最终提示词、实现迭代过程与遇到的问题。
课程明确要求报告中说明「每个练习的 spec 是怎么写的、prompt 是怎么写的」，
以及「使用提示词让 AI 生成代码的过程中出现了什么 bug、以及你是怎么解决的」。
-->

### 前置：补回 lab2 的实验代码

**负责人：** 2413628-黄乐仁（`page_init()`）、2410868-孙锘梵（Best-Fit）

**说明：** 实验指导书要求「使用你此前设计的提示词生成相应实验代码，并将生成结果填写到本实验代码中标有'lab2'注释的对应位置」。本实验的代码中有两处 `//lab2 请补充你在lab2中的代码`：

| 位置 | 需要补回的内容 |
|---|---|
| `kern/mm/pmm.c`（第 84 行附近） | lab2 的 `page_init()` |
| `kern/mm/best_fit_pmm.c`（第 7 行附近） | lab2 的 Best-Fit 管理模块（`free_area` + 五个管理函数 + `best_fit_pmm_manager` 注册结构；**两个测试函数已由实验提供，不要重复实现**） |

**最终提示词：** 见 [`report/prompt.md`](./prompt.md) 对应小节（即 lab2 使用过的提示词）。

**补回后的验证：** 代码应能编译通过，且 `make grade` 中前两项（`page init` / `best fit`）仍为 OK。

**Best-Fit 补回说明（2410868-孙锘梵）：**

`kern/mm/best_fit_pmm.c` 中的 `basic_check()` 与 `best_fit_check()` 两个测试函数已由实验框架提供，补回时只需补齐缺失的三部分：

| 补回内容 | 说明 |
|---|---|
| 空闲区结构 | `static free_area_t free_area;`，并用两个宏把 `free_list` / `nr_free` 映射到它的成员；已提供的测试函数正是通过这两个名字访问空闲链表，缺了会直接编译失败 |
| 五个管理函数 | `best_fit_init`、`best_fit_init_memmap`、`best_fit_alloc_pages`、`best_fit_free_pages`、`best_fit_nr_free_pages` |
| 管理器注册 | `const struct pmm_manager best_fit_pmm_manager`，把五个函数指针与 `.check = best_fit_check` 绑定 |

与 First-Fit 的实质差别只有 `best_fit_alloc_pages()` 的选块策略：First-Fit 遇到第一个 `property >= n` 的块就返回，Best-Fit 则遍历整条空闲链表，在所有满足条件的块中挑 `property` 最小的那一块，以减少本次分配对空闲区造成的碎片。

最终提示词见 [`report/prompt.md`](./prompt.md) 第二节「提示词 0」。

---

### 功能模块一：Trap 上下文恢复与 Trap 入口功能

**负责人：** 2413628-黄乐仁

**需要实现/修改的汇编功能（`kern/trap/trapentry.S`）：**

```asm
RESTORE_ALL     # 与已提供的 SAVE_ALL 对应的恢复逻辑
__alltraps      # 在 SAVE_ALL 之后调用 C 语言 trap(struct trapframe *tf)
```

**功能说明：**

本模块负责完成 Trap 从「进入内核」到「返回原执行流」之间的底层上下文管理。`SAVE_ALL` 已经负责把 Trap 发生时的处理器状态保存到当前内核栈上的 Trap frame 中，需要根据这一已有的保存关系设计提示词，让 AI 完成对应的 `RESTORE_ALL`，使被打断程序的执行状态能够从 Trap frame 中恢复出来。同时，`__alltraps` 需要建立汇编 Trap 入口与 C 语言 `trap(struct trapframe *tf)` 之间的调用关系，使保存后的 Trap frame 能够按照 RISC-V 调用约定正确传递给 `trap()`。

**注意事项（指导书原文要点）：**

- `RESTORE_ALL` 必须与已经提供的 `SAVE_ALL` 保持对应关系，**不能自行假设一个与实际 Trap frame 不一致的数据布局**。
- 特别注意：`SAVE_ALL` 中对 **`x2(sp)` 的处理方式与其他通用寄存器并不完全相同**，因此恢复时不能简单地把所有寄存器按完全相同的方式恢复。
- `__alltraps` 需要保证 Trap frame 的地址能按 RISC-V 调用约定作为 `trap()` 的参数传递，同时不能破坏后续恢复上下文所需的信息。
- **不得修改 `SAVE_ALL`、`struct trapframe` 和 `trap()` 的函数签名。**

**验收标准（来自 `tools/grade.sh`）：**

- `[Test 1] Trap entry and breakpoint return: PASS (35/35)` —— 35 分
- 需要看到输出 `Breakpoint at`

**最终提示词：**

<!-- 见 report/prompt.md；此处粘贴最终定稿 -->

**实现迭代过程：**

<!-- 记录 AI 生成 RESTORE_ALL / __alltraps 时出现的 bug（如寄存器恢复顺序、
     sp 的特殊处理、tf 传参寄存器用错等）以及解决过程 -->

---

### 功能模块二：中断与异常处理功能

**负责人：** 2410868-孙锘梵

**需要实现的函数（`kern/trap/trap.c`）：**

```c
void interrupt_handler(struct trapframe *tf);
void exception_handler(struct trapframe *tf);
```

**功能说明：**

- `interrupt_handler()` 负责处理中断。其中**时钟中断**需要完成：设置下一次时钟事件、时钟计数、统计信息输出（达到规定次数时）、以及累计输出达到规定次数后的系统关机；其他类型的中断只需完成基本的类型区分和相应提示输出；**无法识别的中断**则通过已有的 `print_trapframe(tf)` 输出完整 Trap frame 以辅助调试。
- `exception_handler()` 负责处理实验要求的异常。其中**非法指令异常**和**断点异常**需要输出规定格式的信息，并**根据对应指令的长度正确更新 `tf->epc`**，使 Trap 返回以后不会重复执行已经触发异常的指令；其他未要求具体处理的异常则使用已有的 `print_trapframe(tf)` 作为兜底处理。

**注意事项（指导书原文要点）：**

- 时钟中断**不能只计数而不设置下一次时钟事件**，否则系统只能收到一次时钟中断。
- `ticks` 每收到一次时钟中断必须增加一次；达到 `TICK_NUM` 的整数倍时输出规定格式的统计信息。
- 非法指令与断点异常必须严格按实验规定的输出格式处理，并正确更新 `tf->epc`，否则返回后会**再次执行同一条异常指令形成异常循环**。
- **不同异常对应的指令长度可能不同**，不能假定所有异常都用相同的 `epc` 偏移量，需要结合 RISC-V 指令编码规则判断。
- 未列出的异常类型只需调用 `print_trapframe(tf)` 兜底输出。
- **不得修改函数签名，也不得修改 `trap.c` 中规定以外的其他函数。**

**验收标准（来自 `tools/grade.sh`）：**

- `[Test 2] Illegal instruction return: PASS (25/25)` —— 25 分
  需要看到：`Illegal instruction at`、`Supervisor software interrupt`、`Unknown interrupt: cause 15`、`cause    0x00000014`
- `[Test 3] Sustained supervisor timer: PASS (40/40)` —— 40 分
  需要看到：`100 ticks`、`ticks=200`

**最终提示词：**

两个 handler 各使用一份独立提示词，全文见 [`report/prompt.md`](./prompt.md) 第二节的「提示词 1」「提示词 2」。定稿原文如下。

````markdown
[PROMPT]
**任务**：修改 `kern/trap/trap.c`，在 `//lab3 function2 YOUR CODE` 处实现 `interrupt_handler()`，使内核能区分并处理 S 模式的时钟、软件、外部中断，并对无法识别的中断做兜底输出。

**操作要求**：必须在项目中修改实际文件；新增代码写入标记处且位于 `trap_dispatch()` 之前；`idt_init()`、`print_trapframe()`、`print_ticks()` 等既有函数保持不动；该函数**不得声明为 `static`**。

**输出要求**：使用下方信息，以工程真实定义为准。

[RELY]
```c
// libs/riscv.h
IRQ_S_SOFT   1
IRQ_S_TIMER  5
IRQ_S_EXT    9

// driver/clock.h、trap.c
extern volatile size_t ticks;      // 累计时钟中断次数
void clock_set_next_event(void);   // 安排下一次时钟事件（一次性）
#define TICK_NUM 100
static void print_ticks(void);      // 已提供，输出统计信息

void print_trapframe(struct trapframe *tf);
void cprintf(const char *fmt, ...);
```

```
// scause 最高位为 1 表示中断，原因编号位于低位；
// 入参 tf->cause 按 scause 原值约定，可能带也可能不带最高位。
```

[GUARANTEE]
```c
void interrupt_handler(struct trapframe *tf);   // 不得为 static
```

[SPECIFICATION]
## interrupt_handler
**Pre-Condition**：`tf` 有效，`tf->cause` 为 `scause` 原值。
**Post-Condition**：对应中断处理完毕，函数正常返回；`tf->epc` 不被修改（中断返回后应继续执行被打断的指令）；处理过程中不关闭中断使能、不调用 `panic()`。
**Case 1**（`IRQ_S_TIMER`）：先重新安排下一次时钟事件，再累加 `ticks`；计数每满 `TICK_NUM` 输出一次统计信息。
**Case 2**（`IRQ_S_SOFT`）：输出一行可辨识的 S 模式软件中断提示。
**Case 3**（`IRQ_S_EXT`）：输出一行可辨识的 S 模式外部中断提示。
**Case 4**（其他中断号）：先输出含该中断号十进制值的提示，再打印完整 Trap frame 作为兜底。

**Requirements**：
- 判断类型前必须先剥离 `scause` 的最高位；输出的中断号同样取剥离后的值。
- 时钟分支中「重新安排下一次事件」必须先于其他处理，任何路径都不得漏设，否则时钟中断只到来一次。
````

````markdown
[PROMPT]
**任务**：修改 `kern/trap/trap.c`，在 `//lab3 function2 YOUR CODE` 处实现 `exception_handler()`，按异常原因处理非法指令与断点，并保证处理完成后返回原执行流而不再触发同一条指令。

**操作要求**：必须在项目中修改实际文件；新增代码写入标记处且位于 `trap_dispatch()` 之前；`interrupt_handler()`、`trap_dispatch()`、`trap()` 等既有函数保持不动；该函数**不得声明为 `static`**。

**输出要求**：使用下方信息，以工程真实定义为准。

[RELY]
```c
// trap.h
struct trapframe { struct pushregs gpr; uintptr_t status, epc, badvaddr, cause; };

// libs/riscv.h
CAUSE_ILLEGAL_INSTRUCTION  0x2
CAUSE_BREAKPOINT           0x3

void print_trapframe(struct trapframe *tf);
void cprintf(const char *fmt, ...);
```

```
// 异常 cause 为非负小整数，不需要剥离标志位；sret 会把 sepc 装回 pc。
```

[GUARANTEE]
```c
void exception_handler(struct trapframe *tf);   // 不得为 static
```

[SPECIFICATION]
## exception_handler
**Pre-Condition**：`tf` 有效，`tf->cause` 为异常原因。
**Post-Condition**：非法指令与断点处理完成后，`tf->epc` 已指向触发异常的指令之后；其他异常 `tf->epc` 保持不变；函数正常返回，不调用 `panic()`。
**Case 1**（`CAUSE_ILLEGAL_INSTRUCTION`）：输出含 `tf->epc` 数值的提示，再按该地址处指令的编码决定前进长度并更新 `tf->epc`：最低两位为 `0b11` 是 32 位指令，否则是 16 位压缩指令。
**Case 2**（`CAUSE_BREAKPOINT`）：输出含 `tf->epc` 数值的提示；`ebreak` 为 4 字节指令，`tf->epc` 前进 4。
**Case 3**（其他异常）：不改变 `tf->epc`，打印完整 Trap frame 作为兜底。

**Requirements**：
- 必须更新 `tf->epc`，否则返回后再次执行同一条异常指令，形成异常循环。
- 指令长度只能由编码判定，不能写死偏移量。
- 只有非法指令分支读取 `tf->epc` 处的内存；兜底分支的 Trap frame 可能来自人工构造，不得无条件解引用。
- 打印 64 位地址时使用 64 位格式化符，避免截断。
````

**实现迭代过程：**

**一、提示词的三轮调整**

*第一轮：从一句话到四段式。* 最初的描述只有「实现 `interrupt_handler` 和 `exception_handler`」。问题在于这两个函数唯一的入参是 `tf`，而几乎所有行为都取决于 `scause` 的编码方式与 `trapframe` 的字段语义；这些不写进 `[RELY]`，生成结果里的宏名和字段名就只能靠猜，极容易出现 `sbadaddr` / `badvaddr`、`stvec` / `sepc` 之类对不上的写法。于是把 `struct trapframe` 的布局、`IRQ_S_*` 与 `CAUSE_*` 的取值、`clock_set_next_event()` 与 `volatile size_t ticks` 的声明补入 `[RELY]`。

*第二轮：先合并、再拆分。* 一开始把两个 handler 合成一份「中断与异常处理模块」的提示词，理由是二者共享同一套 `scause` 语义。但合并后 `[RELY]` 里同时堆着中断常量与异常常量，任何一侧都用不到另一半，违背了 `[RELY]` 要「最小」的要求；而且两个 handler 的验收彼此独立。最终拆成两份提示词，各自只带自己需要的宏与函数，上下文都压到 10 行以内。

*第三轮：把体系结构约束从「说明」挪进「规格」。* 三条约束最初只打算写成说明性文字：`scause` 最高位的含义、一次性定时器必须每次重设、`epc` 必须前进。但说明性文字容易被当成建议而忽略，只有写进 `Post-Condition` 与 `Requirements` 才会真正约束生成结果。因此 `interrupt_handler` 把「`tf->epc` 不被修改」写成后置条件，把「续期必须先于其他处理」写成 `Requirements`；`exception_handler` 把「`epc` 已指向触发异常的指令之后」写成后置条件。

**二、代码的分步实现**

两个 handler 没有一次性生成，而是按「先定输入、再填分支」的顺序分四步完成，每步都能单独检查：

1. **先只做 `cause` 归一化**，把 `switch (cause)` 的骨架铺好、分支体留空。这一步的目的是先把「输入到底是什么」定死——`scause` 是原始值，最高位是中断标志，不剥离就无法比较。
2. **填时钟分支**：`clock_set_next_event()` → `ticks++` → 每满 `TICK_NUM` 调用框架已提供的 `print_ticks()`。
3. **填软件中断、外部中断、未知中断**三个分支。
4. **单独实现 `exception_handler`**：非法指令按编码判长、断点固定前进 4 字节、其余走 `print_trapframe()` 兜底。

**三、实现过程中处理的问题**

1. **`%d` 与 64 位实参不匹配。** 未知中断要输出中断号的十进制值，而 `cause` 是 `uintptr_t`（64 位）。变参函数不做类型提升，直接把 64 位值交给 `%d` 属于未定义行为。处理方式是在输出前显式转换为 `int`：`cprintf("Unknown interrupt: cause %d\n", (int)cause)`。
2. **兜底分支不能解引用 `tf->epc`。** 非法指令分支需要读回触发异常的指令来判断长度，但如果把这段读取放在函数入口统一执行，遇到人工构造的 `trapframe`（`epc` 为 0）就会直接访存失败。处理方式是只在非法指令分支内读取，兜底分支只调用 `print_trapframe(tf)`，它仅访问 `tf` 自身的字段。
3. **`-Werror` 下的声明顺序。** 框架开启了 `-Werror`，若把两个 handler 写在 `trap_dispatch()` 之后，会因为隐式声明直接编译失败。处理方式是全部写在 `trap_dispatch()` 之前。
4. **`static` 会破坏外部调用。** 本文件之外的其他代码会直接调用这两个 handler，若误加 `static` 会导致链接失败。这一点已写进两份提示词的 `[GUARANTEE]` 注释中。
5. **指导书要求的「关机」与框架不一致。** 指导书建议在统计输出累计达到 `SHUTDOWN_AFTER` 后调用关机接口，但框架中并没有定义这个阈值宏，只有 `libs/sbi.h` 提供的 `sbi_shutdown()`。而本实验的验收需要在内核运行较长时间后确认时钟计数仍在增长，过早关机会让观测窗口提前结束。综合两点，本实现只完成「续期 → 计数 → 输出」的闭环，未加入关机判断。




---

## 五、测试与验证

**编译与运行（`make qemu`）：**

<!-- 预期看到一行行的 `100 ticks`（print_ticks 每 100 次时钟中断输出一次） -->

![make qemu 运行结果](./images/lab3_qemu.png)

**自动评分（`make grade`）：**

<!-- lab3 的 grade.sh 会用三个不同的宏分别重新编译并运行：
       GRADE_LAB3_CONTEXT  35 分  → [Test 1] Trap entry and breakpoint return: PASS (35/35)
       GRADE_LAB3_ILLEGAL  25 分  → [Test 2] Illegal instruction return: PASS (25/25)
       GRADE_LAB3_TIMER    40 分  → [Test 3] Sustained supervisor timer: PASS (40/40)
     总分 100。截取 Total Score: 100/100 的输出。 -->

![make grade 评分结果](./images/lab3_grade.png)

---

## 六、实验总结与收获

### 对操作系统的理解

<!-- 要求：
     1. 列出本实验中重要的知识点，与对应 OS 原理中的知识点，说明含义、关系和差异
     2. 列出 OS 原理中很重要、但在本实验中没有对应上的知识点
     可参考方向：中断 vs 异常、特权级与 Trap 委托（medeleg/mideleg）、
     上下文切换与 trapframe、stvec/sepc/scause/sstatus、时钟中断与定时器、
     关中断与原子操作、嵌套中断与中断屏蔽 -->

**1. 本实验中重要的知识点，及与 OS 原理中对应知识点的关系**

[待填写]

**2. OS 原理中很重要、但本实验没有对应上的知识点**

[待填写]

### AI 协作开发的经验

<!-- 要求：结合本实验真实经过写。素材方向：
     - 汇编类任务（RESTORE_ALL）给 AI 的上下文要包含 SAVE_ALL 的完整内容与 trapframe 布局
     - sp 的特殊处理是最容易出错的地方
     - epc 修正与指令长度判断
     - 时钟中断"一次性定时器必须每次重设"
     - 本仓库的环境适配（-device loader 在 QEMU 8 下失效） -->

[待填写]

---
