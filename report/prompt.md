# Lab 1 提示词汇总

> **本文件的组织方式**
>
> 本文件按小组成员分部分汇总本实验使用过的提示词：
>
> - **部分一：内核入口与启动链路** —— 负责人 2410868-孙锘梵
> - **部分二：GDB 调试实操** —— 负责人 2413628-黄乐仁
> - **部分三：原理对应与收口** —— 负责人 2412796-杨斯源（组长）
>
> 各部分提示词均由对应负责人提供，统一采用 lab0.5 规定的四段式框架（`[PROMPT]` / `[RELY]` / `[GUARANTEE]` / `[SPECIFICATION]`）。
>
> 另外说明一点：lab1 的实验内容是「最小可执行内核」，**没有任何需要编写或修改代码的练习**——练习 1 是阅读 `kern/init/entry.S` 并回答两道概念题，练习 2 是用 GDB 跟踪启动流程并回答问题。因此本实验的 AI 协作不是「让 AI 生成代码」，而是**让 AI 协助理解启动链路、设计调试方案、梳理 OS 原理对应关系**。

---

## 一、部分一：内核入口与启动链路（2410868-孙锘梵）

以下为部分一负责人本人在本实验过程中实际使用的提示词。

[PROMPT]
看下面这个项目，总结分工部分一工作，可以参照附件里的报告草稿，注意这个草稿内容不保证准确，需要辨别

[RELY]
`lab1-tasks.md`
`report.md`

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
阅读附件，结合刚才的文件，帮我看看 entry.S 里 la sp, bootstacktop 和 tail kern_init 这两条指令，各自完成了什么？

[RELY]
`entry.S`

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
kern_init 是干什么的？还有把 la 和展开后的每条真实指令逐行加个注释

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
把上面的指令展开后得到的指令详细讲讲，并推测答辩考点

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
也就是说第一条指令 la sp, bootstacktop相当于把 bootstacktop 的高位和低位分别写进 sp？

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
再讲讲第二条展开成的这两条

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
下面是报告，你帮我勘误一下我的工作内容，不是我工作内容的部分不用管

[RELY]
`report.md`

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
对这些勘误进行重要性评估

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
练习一先这样，我下面还有啥工作内容？回忆给你的分工手册

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
可以，现在开始反汇编，环境Ubuntu 22.04，给我反汇编相关指令

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
我截了两张图，分析结果正确性，并告诉我咋按要求插入报告

[RELY]
`屏幕截图 2026-10-07 165744.png`
`屏幕截图 2026-10-07 165746.png`

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
我发现截图直接拖进去显示成 ![alt text](<lab1_nm.png>)，看起来预览正常。这样文件在其他机器上图片会正常显示吗

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
为啥不行？我给你的哪个文件要求图片必须在那个目录了？

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
分工文件里部分一的工作帮我对一遍，并审阅报告是否有问题（限我的工作范围）

[RELY]
`report.md`

[GUARANTEE]

[SPECIFICATION]

---

[PROMPT]
现在把我的提示词整理一下，从讨论分工开始，删去和实验无关的问题（包括环境配置）以markdown格式给我，格式要求：[PROMPT]做什么、改哪里、怎么改[RELY]可以依赖什么——宏定义、数据结构、已有函数[GUARANTEE]必须实现或修改什么——需要交付的函数列表[SPECIFICATION]每个函数的详细行为规格。不需要排版和题目等，其中我们有附件的对话把附件名写进RELY，剩下俩先空着，包括这句

[RELY]

[GUARANTEE]

[SPECIFICATION]

---

## 二、部分二：GDB 调试实操（2413628-黄乐仁）

以下为部分二负责人本人在本实验过程中实际使用的提示词。

### 练习2（GDB 调试实操）提示词（细分版）

> 负责部分：报告第四节「练习2」+ 第五节两张截图 + `report/images/`
> 协作模式：本人主导目标与决策，AI 辅助执行与解释；本练习不修改任何代码，只做运行、调试、观察与记录。
> 说明：把练习2 拆成 7 个细分提示词，每一条都对应一个可独立交付、可验收的子任务，全部遵循 lab0.5 四段式框架（`[PROMPT]` / `[RELY]` / `[GUARANTEE]` / `[SPECIFICATION]`）。实测值以你亲手跑出的输出为准，文中标注的值是参考基准。

---

#### 提示词 1：明确练习2 的任务与交付边界

````markdown
[PROMPT]
**任务**：确认我负责「练习2：GDB 调试实操」的具体交付内容与边界，避免越界改动他人负责的部分。

**操作要求**：先通读任务分工文档，确认练习2 要交付什么、哪些内容属于组员/组长，再动手。

**输出要求**：
1. 我负责的部分名称与对应报告章节；
2. 该部分要交付的具体文件清单（章节、截图、路径）；
3. 明确列出「我不应改动」的内容。

[RELY]
- 任务分工文档（`docs/lab1-tasks.md` 第二节）
- 实验报告模板 `report/report.md`

[GUARANTEE]
必须产出：明确的「我负责 / 我不负责」清单。

[SPECIFICATION]
**Pre-Condition**：尚未开始任何运行或改动。
**Post-Condition**：本人职责与交付物边界清晰，后续操作不越界。
**Requirements**：
- 必须确认我负责「部分二：练习2 GDB 调试实操」；
- 必须列出交付物：报告第四节「练习2」、第五节两张截图、`report/images/`；
- 必须指出第二节环境、第六节总结、`prompt.md` 汇总为组长（部分三）负责，不在我改动范围；
- 必须指出第三节逻辑分析、练习1 为组员（部分一）负责，我不改动。
````

---

#### 提示词 2：编译并运行，确认启动输出

````markdown
[PROMPT]
**任务**：编译内核并跑通启动，确认能看到目标输出，为后续 GDB 调试与截图做准备。

**操作要求**：给出从编译到运行的完整命令与每步预期输出，我亲手在终端执行并核对。

**输出要求**：
1. 编译命令与预期结果；
2. 运行命令与预期输出内容；
3. 如何退出 QEMU。

[RELY]
- 代码在 `code/` 目录（含 `Makefile`，提供 `make` / `make qemu` 等目标）
- 环境：riscv64 工具链已就绪

[GUARANTEE]
必须产出：可复制的命令序列，以及「预期看到什么」的判断标准。

[SPECIFICATION]
**Pre-Condition**：代码已就位，环境可用，尚未编译。
**Post-Condition**：`make qemu` 能看到 `(THU.CST) os is loading ...` 后停住（内核 `while(1)` 死循环）。
**Requirements**：
- 编译命令 `make`（必要时先 `make clean` 再全新编译），确认生成 `bin/kernel` 与 `bin/ucore.img`；
- 运行命令 `make qemu`：先打印 OpenSBI banner（`Firmware Base: 0x80000000`、`Domain0 Next Address: 0x80200000`），再打印 `(THU.CST) os is loading ...`，之后停住不再退出；
- 必须说明退出方式：按住 `Ctrl` 再按 `a`，都松开后单独按 `x`；
- 输出不符合预期时要能给出排查方向（编译报错、工具链缺失、串口无输出等）。
````

---

#### 提示词 3：搭建 GDB 双终端调试环境

````markdown
[PROMPT]
**任务**：用 `make debug` + `make gdb` 搭起「QEMU 调试桩 + GDB 前端」的双终端环境，并解释这两个目标背后的参数含义。

**操作要求**：说明为什么必须两个终端、两个命令的先后顺序、各自做了什么。

**输出要求**：
1. 终端 A / 终端 B 分别执行什么命令；
2. `make debug` 的 QEMU 参数（`-s`、`-S`）含义；
3. 连接成功/失败的表现与排查。

[RELY]
- `Makefile` 中的 `debug`、`gdb` 目标
- gdb-multiarch 已安装，可调试 riscv64 目标

[GUARANTEE]
必须产出：可复制的双终端启动步骤，以及 `-s`/`-S` 的准确解释。

[SPECIFICATION]
**Pre-Condition**：已完成 `make` 编译。
**Post-Condition**：终端 A 的 QEMU 暂停在复位状态等待连接，终端 B 的 GDB 成功连上并停在 `0x1000`。
**Requirements**：
- 终端 A 执行 `make debug`（QEMU 加 `-s -S`，占住前台等待连接）；终端 B 执行 `make gdb` 连接 1234 端口；
- 必须解释：`-s` = 在 1234 端口开放 GDB 调试桩（等价 `-gdb tcp::1234`）；`-S` = CPU 启动即暂停、不执行任何指令；
- 必须说明两个终端不可颠倒的原因（QEMU 一直占着前台跑，gdb 是另一个进程去连它）；
- 连接成功判据：GDB 显示 `The target architecture is set to "riscv:rv64"` 且停在 `0x1000`。
````

---

#### 提示词 4：观察复位向量——复位 pc 与最初 6 条指令

````markdown
[PROMPT]
**任务**：在 GDB 中观察加电复位后的初始状态，确认复位 pc 与复位向量前几条指令，并逐条解释它们做什么。

**操作要求**：给出 GDB 观察命令，我亲手执行并记录真实输出。

**输出要求**：
1. `info registers pc` 的输出与含义；
2. `x/10i 0x1000` 的输出与逐条注释；
3. 「加电后最初几条指令在哪、做什么」的结论。

[RELY]
- 已连上 GDB、停在复位状态
- 背景：RISC-V 加电复位时硬件把 PC 强制置为 `0x1000`

[GUARANTEE]
必须产出：基于本人 `x/10i 0x1000` 真实输出的逐条解释，以及复位 pc 的实测值。

[SPECIFICATION]
**Pre-Condition**：GDB 已连接，目标停在复位后的初始状态。
**Post-Condition**：记录下复位 pc 与复位向量 6 条指令的完整反汇编与解释。
**Requirements**：
- `info registers pc` 应看到 `pc = 0x1000`（RISC-V 复位地址硬性约定）；
- `x/10i 0x1000` 应看到 6 条有效指令：
  `0x1000: auipc t0,0x0`（t0=0x1000 取基址）→ `0x1004: addi a2,t0,40`（a2=0x1028 设备树）→ `0x1008: csrr a0,mhartid`（a0=hart id）→ `0x100c: ld a1,32(t0)`（a1=设备树地址）→ `0x1010: ld t0,24(t0)`（t0=0x80000000，OpenSBI）→ `0x1014: jr t0`（跳 OpenSBI）；
- 必须指出 `0x1018` 之后反汇编为 `unimp`，那是数据（OpenSBI 地址、设备树信息）不是代码；
- 结论一句话：前 6 条指令「备好参数（hart id、设备树），然后跳到 OpenSBI」。
````

---

#### 提示词 5：设断点跟踪内核入口与 sp 变化

````markdown
[PROMPT]
**任务**：在 `0x80200000` 设断点，抓「OpenSBI → 内核入口」的控制权交接，观察 pc 与 sp 的变化，并解释为什么用断点而不是监视点。

**操作要求**：给出断点设置与观察命令，我亲手执行并记录真实值。

**输出要求**：
1. `x/3i 0x80200000` 看到的 `kern_entry` 前几条指令；
2. 断点命中时的 `pc`、`sp` 实测值；
3. `si` 单步后 `pc`、`sp` 的实测值与变化原因；
4. 「为什么用 `b` 不用 `watch`」的解释。

[RELY]
- 背景：OpenSBI 完成硬件初始化后跳到 `0x80200000` 把控制权交给内核（S 模式）
- 内核入口在 `kern/init/entry.S` 的 `kern_entry`：先立栈（`la sp, bootstacktop`）再跳 C 入口 `kern_init`

[GUARANTEE]
必须产出：断点命中的实测 pc/sp、单步后的实测 pc/sp，以及 sp 变化的原理解释。

[SPECIFICATION]
**Pre-Condition**：GDB 已连接，处于复位状态，尚未设置断点。
**Post-Condition**：记录下内核入口交接时 pc/sp 的完整变化过程。
**Requirements**：
- `x/3i 0x80200000` 应看到 `kern_entry` 前几条指令（`auipc sp,0x3` / `mv sp,sp` / `j kern_init`），即 `la sp, bootstacktop` 的展开；
- 必须用 `b *0x80200000`（明确禁用 `watch *0x80200000`），理由：内核镜像是 QEMU 在机器初始化阶段就写进内存的，OpenSBI 运行期不加载它，watch 监视「写入」永不触发，只有 `b` 能抓到控制权移交那一刻；
- `continue` 后断点命中 `kern_entry`：`pc = 0x80200000`，`sp` 为 OpenSBI 移交时的残留值（本环境参考 `0x80017ee0`）；
- `si` 执行 `la sp, bootstacktop` 后：`pc = 0x80200004`、`sp = 0x80203000`（bootstacktop）；
- 必须解释 sp 变化：残留值来自 OpenSBI（固件对内核栈无约定），内核必须自己把 sp 设到 bootstacktop 才有可用栈。
````

---

#### 提示词 6：回答练习2 问题并填写观察结果表

````markdown
[PROMPT]
**任务**：把前面实测到的内容整理成报告第四节「练习2」的正式文字：回答题目问题 + 填写 6 行观察结果表。

**操作要求**：所有结论必须来自我亲手跑出的真实输出，不套用模板值、不编造。

**输出要求**：
1. 「加电后最初几条指令在哪、做什么」的完整答案；
2. 观察结果表 6 行全部填实测值；
3. 每个关键值的一句话解释。

[RELY]
- 前面几步记录的真实输出（复位 pc、`x/10i 0x1000`、断点 pc/sp、单步后 pc/sp）
- 报告模板第四节格式

[GUARANTEE]
必须产出：可直接粘贴进报告的「问题解答 + 6 行实测观察表」，数据真实可追溯。

[SPECIFICATION]
**Pre-Condition**：已完成全部 GDB 观察，手上有真实输出。
**Post-Condition**：报告练习2 内容齐全，观察表无空项、无编造。
**Requirements**：
- 问题解答必须基于本人 `x/10i 0x1000` 的真实输出：最初几条指令在 `0x1000`（MROM 复位向量），逐条说明作用后概括为「备参数、跳 OpenSBI」；
- 观察表 6 行至少覆盖：复位 pc、复位向量指令、`si` 单步、内核入口 `x/3i`、断点命中 pc/sp、单步立栈后 pc/sp；
- 每条必须标注是「实测值」并给出寄存器/地址含义；
- 与组长/组员章节（第二节环境、练习1）保持边界，不重复、不冲突。
````

---

#### 附：练习2 交付清单速查

| 交付物 | 内容 | 关键实测值（参考，以实际为准） |
|---|---|---|
| 报告第四节 练习2 | 问题解答 + 6 行观察表 | 复位 pc=0x1000；复位向量 6 条指令（0x1000~0x1014） |
| 报告第五节 截图1 | `report/images/lab1_qemu.png` | 能看清 `(THU.CST) os is loading ...` |
| 报告第五节 截图2 | `report/images/lab1_gdb.png` | 断点命中 pc=0x80200000、sp=0x80017ee0；单步后 sp=0x80203000 |

---

## 三、部分三：原理对应与收口（2412796-杨斯源）

### 提示词 1：理解内核入口的两条指令（对应练习 1）

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

### 提示词 2：梳理从加电到内核的完整启动链路（支撑练习 1、练习 2）

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

### 提示词 3：设计 GDB 调试方案（对应练习 2）

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

### 提示词 4：梳理与 OS 原理的对应关系（对应报告第六节）

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

## 四、迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | 提示词 1（入口指令分析） | 首轮回答只停在语义层面，说了"把 `bootstacktop` 的地址装入 `sp`"，但没有落到指令编码，因此无法解释反汇编里为什么是 `auipc sp,0x3` 加 `mv sp,sp` 两条 | 在 `[SPECIFICATION]` 的 `Requirements` 中增加"必须给出伪指令展开后的真实指令，并说明为什么要拆成高位与低位两条" |
| 2 | 提示词 3（GDB 调试方案） | 首轮方案直接沿用了指导书推荐的 `watch *0x80200000`，实测该监视点始终不触发——因为内核是 QEMU 在机器初始化阶段就写入内存的，并不存在"被写入的瞬间" | 在 `[RELY]` 中补充"内核镜像由 QEMU 在机器初始化阶段预先加载，OpenSBI 不负责加载"这一实测事实；并在 `[SPECIFICATION]` 中把要求改为"给出在当前 QEMU + OpenSBI 版本下可实际触发的验证手段"，最终改用 `b *0x80200000` |
| 3 | 提示词 2（启动链路梳理） | 首轮回答把"OpenSBI 把内核加载到 `0x80200000`"当成既成事实，与 `x/3i 0x80200000` 的实测结果相矛盾（复位时刻该地址上已有内核指令） | 在 `[RELY]` 中补充 `Domain0 Next Address` 字段的含义、以及 `-kernel` 与 `-device loader` 的区别；并在 `[SPECIFICATION]` 的 `Case 2` 中明确要求区分"QEMU 预加载"与"固件加载" |

> 说明：本实验的提示词均为分析类任务，不生成代码，因此不存在"生成的代码编译不过"这类问题；迭代主要发生在"回答是否足够具体、是否落实到本实验的代码细节"上。
