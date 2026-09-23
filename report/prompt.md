# Lab 1 提示词汇总

> **本文件的定位**
>
> lab1 的实验内容是「最小可执行内核」，**没有任何需要编写或修改代码的练习**：练习 1 是阅读 `kern/init/entry.S` 并回答两道概念题，练习 2 是用 GDB 跟踪启动流程并回答问题。因此本实验的 AI 协作不是「让 AI 生成代码」，而是**让 AI 协助理解启动链路、设计调试方案、梳理 OS 原理对应关系**。
>
> 下面按 lab0.5 规定的四段式框架（`[PROMPT]` / `[RELY]` / `[GUARANTEE]` / `[SPECIFICATION]`）记录本实验实际使用的提示词。由于任务性质是分析而非改代码，框架中的 `[GUARANTEE]` 一节相应地表示「必须产出的结论」，`[SPECIFICATION]` 表示「结论必须满足的规格」。

---

## 提示词 1：理解内核入口的两条指令（对应练习 1）

````markdown
[PROMPT]
**任务**：阅读 `kern/init/entry.S`，逐条解释内核入口 `kern_entry` 中两条指令的作用与设计目的。

**操作要求**：这是一道阅读分析题，不需要修改任何代码。请基于给出的源码逐条分析，不要泛泛而谈。

**输出要求**：
1. `la sp, bootstacktop` 完成什么操作、目的是什么；
2. `tail kern_init` 完成什么操作、目的是什么；
3. 说明这两条指令为什么必须按这个顺序出现；
4. 指出 `la` 和 `tail` 是伪指令，并给出它们展开后的真实指令。

[RELY]
// 内核入口汇编（kern/init/entry.S）
.section .text,"ax",%progbits
    .globl kern_entry
kern_entry:
    la sp, bootstacktop
    tail kern_init

.section .data
    .align PGSHIFT
    .global bootstack
bootstack:
    .space KSTACKSIZE
    .global bootstacktop
bootstacktop:

// 链接脚本中的相关约定（tools/kernel.ld）
ENTRY(kern_entry)                  // 入口点为 kern_entry
BASE_ADDRESS = 0x80200000;         // 内核加载基址
.text : {
    *(.text.kern_entry)            // kern_entry 被放在 .text 最开头
    *(.text .stub .text.* .gnu.linkonce.t.*)
}

// 后续 C 语言入口（kern/init/init.c）
int kern_init(void) __attribute__((noreturn));

[GUARANTEE]
必须产出：
- 对 `la sp, bootstacktop` 的完整解释（操作 + 目的）
- 对 `tail kern_init` 的完整解释（操作 + 目的）
- 两条指令的先后顺序的必要性分析
- 伪指令展开后的真实指令形式

[SPECIFICATION]
## 针对 `la sp, bootstacktop`
**Pre-Condition**：内核刚被 OpenSBI 加载到 0x80200000，CPU 开始执行 kern_entry 的第一条指令；此时 sp 中没有指向任何合法的内核栈。

**Post-Condition**：sp 中保存了符号 bootstacktop 的绝对地址；此后压栈操作会从该地址向低地址增长。

**Case 1（正常情况）**：符号 bootstacktop 在链接时被确定为 bootstack 数组之后的高地址处；`la` 展开为 auipc + addi，把该绝对地址装入 sp。

**Requirements**：
- 必须指出 `la` 是伪指令，并说明它展开为 `auipc` 与 `addi` 两条指令；
- 必须解释 bootstack 与 bootstacktop 的语义（bootstacktop 是栈的高地址端点，即"栈底/空栈状态"）；
- 必须结合「RISC-V 栈向低地址增长」说明为什么要把 sp 设到高地址端而不是低地址端；
- 必须说明为什么进入 C 代码前必须有栈（函数调用需保存返回地址、局部变量、callee-saved 寄存器）。

## 针对 `tail kern_init`
**Pre-Condition**：sp 已指向合法的内核栈顶；kern_entry 已完成自己的使命（立栈）。

**Post-Condition**：控制权转移到 kern_init；返回地址寄存器未被写入（返回地址被丢弃），且不向刚建立的栈压入任何返回地址帧。

**Case 1（正常情况）**：`tail` 展开为 auipc + jalr，且 jalr 的目标寄存器是 x0，因此返回地址被丢弃。

**Requirements**：
- 必须指出 `tail` 是伪指令，并说明它展开为 auipc + jalr，以及目标寄存器为 x0 这一关键细节；
- 必须解释「不保存返回地址」与 kern_init 被声明为 `__attribute__((noreturn))` 之间的对应关系；
- 必须说明为什么这里用 tail 而不是 call（语义表达 + 不在空栈上压入无用帧）。
````

---

## 提示词 2：梳理从加电到内核的完整启动链路（支撑练习 1、练习 2）

````markdown
[PROMPT]
**任务**：梳理 QEMU 模拟的 64 位 RISC-V 机器从加电到内核第一条指令执行这段时间内，控制权经历了哪些阶段、每一阶段各自由谁负责。

**操作要求**：不需要修改代码。请按时间顺序说明各阶段的执行主体、所在地址区间和职责边界，并明确指出"内核的代码从哪一刻开始执行"。

**输出要求**：给出分阶段的执行流说明，并标明每一阶段的地址（复位地址、固件地址、内核加载地址）。

[RELY]
- 内核链接脚本中 BASE_ADDRESS = 0x80200000
- 内核镜像经 objcopy 处理后为纯二进制 bin/ucore.img
- QEMU 启动参数（Makefile 中 qemu 目标）：
  qemu-system-riscv64 \
      -machine virt \
      -nographic \
      -bios default \
      -device loader,file=$(UCOREIMG),addr=0x80200000
- RISC-V 特权级：U（用户）/ S（内核代码运行的特权级）/ M（固件运行的特权级）

[GUARANTEE]
必须产出：
- 分阶段的启动流程说明（至少覆盖：复位 → 固件初始化 → 加载内核 → 跳转内核）
- 每一阶段的执行主体与地址
- 对 `-bios default` 含义的解释

[SPECIFICATION]
## 启动链路说明
**Pre-Condition**：QEMU 已启动，虚拟 CPU 处于复位状态。

**Post-Condition**：产生一份自加电至 0x80200000 的分阶段说明，可作为练习 2 设置 GDB 断点的依据。

**Case 1（复位阶段）**：CPU 从复位地址取指，执行的是固件的最初几条指令，而非内核代码。

**Case 2（固件主初始化）**：固件完成主要硬件初始化，并把内核镜像加载到 0x80200000。

**Case 3（移交内核）**：固件跳转到 0x80200000，控制权移交，内核从 kern_entry 开始执行。

**Requirements**：
- 必须明确区分"固件代码"与"内核代码"的地址边界；
- 必须解释为什么练习 2 中可以在 `0x80200000` 处下断点来验证控制权移交；
- 必须说明 `-bios default` 指定的是 QEMU 自带的 OpenSBI 固件。
````

---

## 提示词 3：设计 GDB 调试方案（对应练习 2）

````markdown
[PROMPT]
**任务**：为"用 GDB 跟踪 QEMU 模拟的 RISC-V 从加电到内核第一条指令（跳转到 0x80200000）"设计一套可执行的调试方案。

**操作要求**：不需要修改代码。请给出具体的 GDB 命令序列，并说明每条命令的作用；方案要避免在固件中单步跟踪大量代码。

**输出要求**：
1. 启动调试环境所需的命令；
2. 一套按阶段组织的 GDB 命令序列（含注释说明每条命令的作用）；
3. 需要记录哪些观察点才能回答"加电后最初执行的几条指令位于什么地址、完成了哪些功能"。

[RELY]
- Makefile 中已提供调试相关目标：
    make debug   # QEMU 带 -S（CPU 启动即暂停）和 -s（开放 gdb 端口 1234）
    make gdb     # gdb 连接 localhost:1234
- 指导书提示的三个阶段：CPU 从复位地址（0x1000）执行固件汇编 → 固件主初始化并把内核加载到 0x80200000 → 固件跳转 0x80200000 移交控制权
- 可用手段：断点（b *addr）、监视点（watch *addr）、单步（si）、查看寄存器（info registers）、反汇编（x/Ni addr）

[GUARANTEE]
必须产出：
- 一套完整、可直接执行的 GDB 命令序列
- 每个观察点的说明
- 说明为什么用 `watch *0x80200000` 比单步跟踪更高效

[SPECIFICATION]
## 调试方案
**Pre-Condition**：内核已编译产出 bin/kernel 与 bin/ucore.img。

**Post-Condition**：能够在不逐条跟踪固件的前提下，记录到复位地址处的指令、内核加载瞬间、以及控制权移交到 0x80200000 的证据。

**Case 1（查看复位状态）**：连接 GDB 后，读取 pc 初值，并反汇编复位地址处的前若干条指令。

**Case 2（跳过大段固件代码）**：用 `watch *0x80200000` 在"内核镜像被写入目标地址"这一时刻中断，从而避免在固件里单步。

**Case 3（确认控制权移交）**：用 `b *0x80200000` 在移交处中断，读取 pc 与 sp，确认第一条内核指令是 `la sp, bootstacktop`。

**Requirements**：
- 必须包含 `info registers pc` 以记录复位后的 pc 初值；
- 必须包含对复位地址处指令的反汇编，以回答"最初几条指令位于什么地址、做了什么"；
- 必须给出"如何确认内核第一条指令"的验证手段；
- 命令序列要能实际执行，不能只给概念描述。
````

---

## 提示词 4：梳理与 OS 原理的对应关系（对应报告第六节）

````markdown
[PROMPT]
**任务**：把 lab1 涉及的知识点与操作系统原理课程中的知识点做对应，并找出原理中重要但本实验没有覆盖到的内容。

**操作要求**：不要罗列名词，每一项都要说明二者**含义上的关系与差异**；"没有对应上的知识点"要说明为什么 lab1 里不存在。

**输出要求**：
1. 一张对应表：本实验知识点 | OS 原理知识点 | 二者的含义、关系与差异；
2. 一份"原理中重要但本实验未覆盖"的清单，每项附一句原因。

[RELY]
lab1 的实际范围（请严格以此为界，不要扩写到后续实验）：
- 使用链接脚本描述内核内存布局（.text / .rodata / .data / .bss / stack / heap）
- 内核入口汇编建立内核栈后移交 C 代码
- 交叉编译、链接、objcopy 生成内核镜像
- OpenSBI 作为固件完成加载与跳转
- 内核侧自实现 memset / cprintf，经 ecall 调用 SBI 的 console_putchar 输出
- 使用 QEMU + GDB 调试启动流程
- 本实验为单核、单执行流、无中断、无分页、无进程概念

[GUARANTEE]
必须产出：
- 对应表（每行含"关系与差异"列）
- 未覆盖知识点清单（含原因）

[SPECIFICATION]
## 对应关系梳理
**Pre-Condition**：已理解 lab1 的完整启动链路。

**Post-Condition**：产出对应表与未覆盖清单，且每一项都能落实到 lab1 的具体代码或行为。

**Case 1（有对应）**：指出本实验知识点在原理中的对应概念，并说明"关系（相同的设计思想）"与"差异（裸机/无抽象层的简化）"。

**Case 2（无对应）**：列出原理中重要但 lab1 未涉及的知识点，并说明 lab1 为什么不需要它。

**Requirements**：
- 每一条"差异"必须具体到 lab1 的实现细节（如"全部使用物理地址，尚无页表"）；
- 未覆盖清单至少包含：虚拟内存与分页、进程与线程抽象、中断与异常处理、并发与同步、文件系统与设备抽象、调度；
- 不得把后续 lab 才会出现的内容写成 lab1 已覆盖。
````

---

## 迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | 提示词 1 | [待补：结合实际情况记录] | [待补] |
| 2 | 提示词 3 | [待补：例如首次给 GDB 方案时只给了概念、没给可执行命令序列，补充了"必须给出完整命令序列"的要求] | [待补] |

> 说明：本实验的提示词均为分析类任务，不生成代码，因此不存在"生成的代码编译不过"这类问题；迭代主要发生在"回答是否足够具体、是否落实到本实验的代码细节"上。
