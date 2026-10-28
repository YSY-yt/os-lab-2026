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
| 2410868-孙锘梵 | [待本人确认] | [待本人确认] | |

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

<!-- 见 report/prompt.md；此处粘贴最终定稿 -->

**实现迭代过程：**

<!-- 记录 AI 生成 handler 时出现的 bug（如 epc 偏移量固定写死导致异常循环、
     时钟中断忘记重新 arm、ticks 类型/原子性问题等）以及解决过程 -->

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
