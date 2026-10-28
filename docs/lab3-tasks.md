# Lab 3 任务分工说明

> 本文件是本小组的内部工作文档，用于明确 lab3 三个部分的负责人、具体工作内容与交付要求。
> 仓库：<https://github.com/YSY-yt/os-lab-2026>　分支：`lab3`
> 实验指导书：<http://oslab.mobisys.top/lab2026/_book/lab3/lab3.html>

---

## 零、先读这一节

### 0.1 分工总览

| | 部分一 | 部分二 | 部分三 |
|---|---|---|---|
| **主题** | Trap 上下文恢复与 Trap 入口 | 中断与异常处理 | 收口与验证 |
| **覆盖的练习** | 功能模块一 | 功能模块二 | —— |
| **要写的代码** | `kern/trap/trapentry.S` 的 `RESTORE_ALL` + `__alltraps` | `kern/trap/trap.c` 的两个 handler | 无 |
| **还要补回** | lab2 的 `page_init()`（`kern/mm/pmm.c`） | lab2 的 Best-Fit（`kern/mm/best_fit_pmm.c`） | 无 |
| **对应评分** | `[Test 1]` **35 分** | `[Test 2]` **25 分** + `[Test 3]` **40 分** | 全量验证 |
| **难度** | 中（汇编） | 中（C） | 中（汇总） |
| **负责人** | 2413628-黄乐仁 | 2410868-孙锘梵 | 2412796-杨斯源（组长） |

**依赖关系**：部分一与部分二**互相耦合**——两者的前后置代码都补完，整棵树才能编译运行（详见 §0.5）。部分三在两者完成后做汇总与验证。

**本次实验没有 Challenge、没有选做题。** 指导书里唯一提到 challenge 的地方是 `kern/init/init.c` 里一行被注释掉的 `lab3_switch_test()`，文档中没有描述、也没有计分。

### 0.2 提交规范（老师要求，不可违反）

- 一次实验一个分支：lab3 的成果**全部提交到 `lab3` 分支**，不要提交到 `main`。
- `lab3` 分支根目录下必须是两个文件夹：`code/` 与 `report/`。
- `report/` 下必须包含 `report.md`、`prompt.md`、`images/`。
- 报告图片一律用相对路径，例如 `./images/lab3_grade.png`。
- **保留 `lab3` 与 `YOUR CODE` 注释，并把 `YOUR CODE` 替换为自己的学号。**

### 0.3 环境：已经替你们适配好了

**不需要重复做环境适配**——仓库里的代码已经改好，直接 `make` 即可：

| 已做的改动 | 原因 |
|---|---|
| `Makefile` 的 `qemu`/`debug`/`test`：`-device loader,...` → **`-kernel $(UCOREIMG)`** | QEMU ≥ 8 自带 OpenSBI 是 `fw_dynamic` 变体，跳转地址必须由 QEMU 经 `-kernel` 告知；否则 `Domain0 Next Address` 为 `0x0`，**内核永不执行** |
| `tools/grade.sh`：启动参数同样改 `-kernel` | 同上。**不改则三个测试全部 0 分** |
| `Makefile` 的 `gdb` 目标改自动探测 | 原硬编码的 `riscv64-unknown-elf-gdb` 在 apt 仓库中没有 |

**三条避坑提醒：**

1. **必须在交互式 Ubuntu 终端里跑 `make`**（非交互 shell 拿不到工具链 `PATH`）。
2. **退出 QEMU**：按住 `Ctrl` 再按 `a`，都松开后单独按 `x`。
3. **lab3 的代码交付时编译不过**——`interrupt_handler` / `exception_handler` 未声明、`page_init` 与 Best-Fit 未补回。**这是预期的**，正好是任务本身。

### 0.4 自动评分是怎么打的

`make grade` 会**用三个不同的宏各重新编译一次**并各跑一次 QEMU，在串口输出里匹配固定字符串：

| 测试 | 宏 | 分值 | 通过的标志 |
|---|---|---|---|
| Trap 入口、寄存器恢复、断点 EPC | `GRADE_LAB3_CONTEXT` | **35** | `[Test 1] Trap entry and breakpoint return: PASS (35/35)` |
| 非法指令 EPC 前进与返回 | `GRADE_LAB3_ILLEGAL` | **25** | `[Test 2] Illegal instruction return: PASS (25/25)` |
| 持续时钟中断 | `GRADE_LAB3_TIMER` | **40** | `[Test 3] Sustained supervisor timer: PASS (40/40)` |

**总分 100 分。**

> `grade.sh` 还会额外检查一些输出片段，**这些字符串就是你的实现要打印的内容**：
> - Test 1：`Breakpoint at`
> - Test 2：`Illegal instruction at`、`Supervisor software interrupt`、`Unknown interrupt: cause 15`、`cause    0x00000014`
> - Test 3：`100 ticks`、`ticks=200`

### 0.5 ⚠️ 部分一与部分二互相耦合

**（1）代码树在两者都完成前编译不过**

- `trap.c` 调用了 `interrupt_handler` / `exception_handler`（部分二）——未实现则**编译失败**；
- `pmm.c` 调用了 `page_init()`（部分一要补回）、`best_fit_pmm.c` 缺管理函数（部分二要补回）——未补则**编译/链接失败**。

所以**任何一方单独改完都跑不起来**，必须两边都补完才能 `make`。

**（2）两人都会改 `kern/trap/` 以外的文件**

部分一改 `kern/mm/pmm.c` 与 `kern/trap/trapentry.S`；部分二改 `kern/mm/best_fit_pmm.c` 与 `kern/trap/trap.c`。**提交前务必先 `git pull`。**

**建议的协作方式：**

1. **先各自把自己的 lab2 代码补回**（这两步是机械性的，但能让树尽快可编译）；
2. 再各自实现自己的功能模块；
3. **最后一起做集成验证**：`git pull` → `make grade` → 看到 **100/100** 才算完。

### 0.6 关于"补回 lab2 代码"

指导书原文：

> **本实验依赖实验2。请使用你此前设计的提示词生成相应实验代码，并将生成结果填写到本实验代码中标有"lab2"注释的对应位置，确保代码能够正确编译。**

代码里有两处 `//lab2 请补充你在lab2中的代码`：

| 位置 | 补什么 | 谁最合适 |
|---|---|---|
| `kern/mm/pmm.c`（约第 84 行） | lab2 的 `page_init()` | lab2 时负责部分一的同学 |
| `kern/mm/best_fit_pmm.c`（约第 7 行） | lab2 的 Best-Fit 管理模块 | lab2 时负责部分二的同学 |

> **注意**：`best_fit_pmm.c` 里 `basic_check` 与 `best_fit_check` 两个测试函数本实验**已经提供**，补回时**不要重复实现**，只需要补 `free_area` 定义、五个管理函数和 `best_fit_pmm_manager` 注册结构。
>
> 参考：lab2 分支的 `code/kern/mm/pmm.c` 与 `code/kern/mm/best_fit_pmm.c` 有完整实现，可用于对照。

---

## 一、部分一：Trap 上下文恢复与 Trap 入口

2413628-黄乐仁

### 1.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| 补回 `page_init()` | `code/kern/mm/pmm.c` 的 `//lab2 ...` 处 | 编译通过 |
| `RESTORE_ALL` | `code/kern/trap/trapentry.S`（`//lab3 function1 YOUR CODE`） | 与 `SAVE_ALL` 严格对应 |
| `__alltraps` 的调用部分 | 同上（第二处 `//lab3 function1 YOUR CODE`） | 按 RISC-V 调用约定传 `tf` |
| 报告「前置」与「功能模块一」 | `report/report.md` | 含最终提示词、迭代过程 |
| 1 份 lab2 提示词 + 1 份 lab3 提示词 | `report/prompt.md` 第一节 | 四段式 |

### 1.2 实现要点

**`RESTORE_ALL`**

必须与 `SAVE_ALL` **严格对应**。关键是读懂 `SAVE_ALL` 做了什么：

```asm
SAVE_ALL:
    csrw sscratch, sp          # 把用户 sp 暂存到 sscratch
    addi sp, sp, -36 * REGBYTES
    STORE x0, 0*REGBYTES(sp)   # 保存 x0..x31（x2 除外，见下）
    ...
    csrrw s0, sscratch, x0     # s0 = 原 sp；sscratch = 0
    csrr s1, sstatus
    csrr s2, sepc
    csrr s3, sbadaddr
    csrr s4, scause
    STORE s0, 2*REGBYTES(sp)   # 把原 sp 存到 trapframe 的 sp 槽
    STORE s1, 32*REGBYTES(sp)  # status
    STORE s2, 33*REGBYTES(sp)  # epc
    STORE s3, 34*REGBYTES(sp)  # badvaddr
    STORE s4, 35*REGBYTES(sp)  # cause
```

> ⚠️ **最容易错的地方**：`x2(sp)` 的处理是特殊的。`SAVE_ALL` 里 sp 被压栈后当作新的栈指针用，真正的原 sp 是通过 `sscratch` 中转、最后存进 `2*REGBYTES(sp)` 槽位的。所以 `RESTORE_ALL` 里**不能简单地 "`LOAD x2, 2*REGBYTES(sp)`" 就完事**——你在恢复 sp 之前还要用 sp 去读其他寄存器。正确做法是先把其他寄存器恢复出来，**最后一步**再用一个临时寄存器把原 sp 取回来写进 sp。

**`__alltraps` 的调用部分**

在 `SAVE_ALL` 之后、`__trapret` 之前，需要：

```asm
mv a0, sp        # 第一个参数 = trapframe 指针（a0 是 RISC-V 的第一个参数寄存器）
jal trap         # 调用 C 语言处理函数
```

> 注意：`trap()` 返回后会落到 `__trapret`，执行 `RESTORE_ALL` 和 `sret`。

### 1.3 验收标准

- `make grade` 中 `[Test 1] Trap entry and breakpoint return: PASS (35/35)`
- 输出里能看到 `Breakpoint at`

### 1.4 完成标准（自检）

- [ ] `[Test 1]` 35/35
- [ ] `SAVE_ALL`、`struct trapframe`、`trap()` 签名**均未修改**
- [ ] `//lab3 function1` 注释保留，`YOUR CODE` 换成学号
- [ ] `prompt.md` 第一节两份提示词已填全
- [ ] 报告里写了 bug 与解决过程

### 1.5 易错点

- ❌ **sp 的恢复顺序**：先恢复其他寄存器，最后恢复 sp。
- ❌ **传参寄存器用错**：RISC-V 第一个参数是 `a0`（x10）。
- ❌ 恢复 CSR 时用错顺序（`sstatus`/`sepc` 要在 `sret` 之前恢复好）。
- ❌ 把 `RESTORE_ALL` 写成与 `SAVE_ALL` 不对称的顺序，导致 trapframe 错位。

---

## 二、部分二：中断与异常处理

2410868-孙锘梵

### 2.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| 补回 Best-Fit | `code/kern/mm/best_fit_pmm.c` 的 `//lab2 ...` 处 | 不重复实现两个测试函数 |
| `interrupt_handler` | `code/kern/trap/trap.c`（`//lab3 function2 YOUR CODE`） | 时钟中断闭环 + 其他中断提示 + 未知兜底 |
| `exception_handler` | 同上 | 非法指令/断点 + epc 修正 + 其他兜底 |
| 报告「功能模块二」 | `report/report.md` | 含最终提示词、迭代过程 |
| 1 份 lab2 提示词 + 1 份 lab3 提示词 | `report/prompt.md` 第二节 | 四段式 |

### 2.2 实现要点

**`interrupt_handler(struct trapframe *tf)`**

| 情形 | 要做什么 |
|---|---|
| **时钟中断**（`IRQ_S_TIMER`） | ① 调 `clock_set_next_event()` **重新设置下一次事件**（一次性定时器，不重设就只中断一次）<br>② `ticks++`<br>③ 当 `ticks % TICK_NUM == 0` 时输出统计（框架已提供 `print_ticks()`，它会打印 `100 ticks`） |
| **软件中断**（`IRQ_S_SOFT`） | 输出 `Supervisor software interrupt` |
| **其他已知中断** | 做类型区分并输出提示 |
| **未知中断** | 输出 `Unknown interrupt: cause %d`（参考格式），并用 `print_trapframe(tf)` 兜底 |

**`exception_handler(struct trapframe *tf)`**

| 情形 | 要做什么 |
|---|---|
| **非法指令**（`CAUSE_ILLEGAL_INSTRUCTION`） | 输出 `Illegal instruction at ...`，并**按该指令长度更新 `tf->epc`**（通常 4 字节；压缩指令为 2 字节——**不能写死**） |
| **断点**（`CAUSE_BREAKPOINT`） | 输出 `Breakpoint at ...`，`tf->epc += 4`（`ebreak` 是 4 字节） |
| **其他异常** | `print_trapframe(tf)` 兜底（例如测试用的 cause 20 = `0x14`） |

> ⚠️ **最容易错的地方**：`tf->epc` 不更新的话，`sret` 之后会**再次执行同一条异常指令**，形成异常循环（测试会一直重复输出）。

### 2.3 验收标准

- `[Test 2] Illegal instruction return: PASS (25/25)`
- `[Test 3] Sustained supervisor timer: PASS (40/40)`
- 输出里能看到：`Illegal instruction at`、`Supervisor software interrupt`、`Unknown interrupt: cause 15`、`cause    0x00000014`、`100 ticks`、`ticks=200`

### 2.4 完成标准（自检）

- [ ] `[Test 2]` 25/25、`[Test 3]` 40/40
- [ ] 未修改 `trap.c` 中规定以外的其他函数
- [ ] `//lab3 function2` 注释保留，`YOUR CODE` 换成学号
- [ ] `prompt.md` 第二节两份提示词已填全
- [ ] 报告里写了 bug 与解决过程

### 2.5 易错点

- ❌ **时钟中断忘记 `clock_set_next_event()`** → 只中断一次，`ticks` 永远到不了 200。
- ❌ **`epc` 不更新或偏移写死** → 异常循环 / 对压缩指令算错。
- ❌ `ticks` 不是 `volatile`（框架里已经是 `volatile size_t ticks;`，别改）。
- ❌ 未知中断/异常直接 `panic`，导致测试的兜底用例失败。

---

## 三、部分三：收口与验证

2412796-杨斯源（组长）

### 3.1 你要交付什么

| 交付物 | 内容 |
|---|---|
| 报告第一节 实验目的 | 已有草稿，核对 |
| 报告第三节 整体逻辑分析 | 3.1 逻辑主线 + 3.2 功能逐步实现 |
| 报告第二节 实验环境 | AI 工具表（收集三人信息）+ 运行环境表 |
| 报告第六节 对 OS 的理解 | 知识点对应表 + 未覆盖知识点 |
| 报告第六节 AI 协作经验 | 结合真实经过写 |
| `prompt.md` 汇总 | 四节齐全 + 迭代记录表 |
| 报告头 | 分工表、完成日期 |
| 最终验证 | `make grade` 100/100 + 截图 + 全文校对 |

### 3.2 完成标准

见下方「四、收口清单」。

---

## 四、收口清单（提交前逐条打勾）

### 代码

- [ ] `cd code && make clean && make` 在 `-Werror` 下无警告通过
- [ ] `make qemu` 能看到一行行的 `100 ticks`
- [ ] `make grade` 得到 **100/100**
- [ ] 两处 `//lab2` 已补回、三处 `//lab3 ... YOUR CODE` 已填并替换为学号
- [ ] `SAVE_ALL`、`struct trapframe`、`trap()` 签名未被修改
- [ ] `bin/`、`obj/` 未被提交

### 报告

- [ ] 六节齐全，**全文无占位符**
- [ ] 每个模块都有「最终提示词」**和**「实现迭代过程」，无空标题
- [ ] 报告要求第 4 条（每个练习的 spec/prompt 怎么写）已覆盖
- [ ] 报告要求第 5 条（AI 生成代码的 bug 与解决）已覆盖
- [ ] 知识点对应表、未覆盖清单已写
- [ ] 三处「负责人」已署名，分工表与完成日期已填

### 提示词与图片

- [ ] `prompt.md` 各部分格式一致，迭代记录表已填
- [ ] `report/images/` 下有 `lab3_qemu.png` 与 `lab3_grade.png`
- [ ] 报告中的图片链接在 GitHub 上能正常显示

### 分支

- [ ] 成果在 `lab3` 分支，根下只有 `code/` 与 `report/`
- [ ] 已 `git push`

---

## 五、命令速查

```bash
cd ~/os-lab-2026/code          # 组长机器上是 /mnt/d/os-lab-2026/code

make                    # 编译
make qemu               # 运行（退出：Ctrl+A 松开再按 x）
make grade              # 自动评分（100 分）
make clean

make debug / make gdb   # 调试（两个终端）
pkill -f qemu-system-riscv64

git pull && git add -A && git commit -m "lab3: ..." && git push
```

---

## 六、参考：lab3 官方要求原文

**实验目的**（`lab3_1_goals.html`）：

> 实验3主要讲解的是中断处理机制。操作系统是计算机系统的监管者，必须能对计算机系统状态的突发变化做出反应……这个过程就是中断处理过程。
> 本章你将学到：riscv 的中断相关知识；中断前后如何进行上下文环境的保存与恢复；处理最简单的断点中断和时钟中断。

**练习**（`lab3_2_1_exercise.html`）——两个功能模块，**均为必做，均要求设计提示词借助 AI 完成**：

> **功能模块一：Trap 上下文恢复与 Trap 入口功能（设计提示词借助 ai 完成）**
> 要求实现的函数或汇编功能有（`kern/trap/trapentry.S`）：`RESTORE_ALL`、`__alltraps`。
> ……本功能模块不要求学生直接按照某一种固定的汇编指令序列实现，而是要求学生根据已有代码、接口和体系结构约束设计 Prompt，让 AI 完成功能实现。
> 整个模块不得修改 SAVE_ALL、struct trapframe 和 trap() 的函数签名。
>
> **功能模块二：中断与异常处理功能（设计提示词借助 ai 完成）**
> `kern/trap/trap.c`：`void interrupt_handler(struct trapframe *tf);`、`void exception_handler(struct trapframe *tf);`
> ……整个模块不得修改函数签名，也不得修改 trap.c 中规定以外的其他函数。

**报告要求**（`lab3_4_lab_requirement.html`）：

> 1. 基于 markdown 格式完成实验报告，以文本说明为主。
> 2. 说明自己对本章节整体逻辑线的理解，即本章节是按照什么顺序、围绕哪些功能逐步实现的。
> 3. 说明自己对每个功能的核心函数或功能模块的理解。
> 4. 说明每个练习的 spec 是怎么写的，prompt 是怎么写的。
> 5. 说明在使用提示词让 AI 生成代码的过程中出现了什么 bug，以及你是怎么解决的。
> 6. 列出你认为本实验中重要的知识点，以及与对应的 OS 原理中的知识点，并简要说明二者的含义、关系和差异。
> 7. 列出你认为 OS 原理中很重要、但在本实验中没有对应上的知识点。
>
> 注意有"lab3"的注释，代码中所有需要完成的地方（challenge 除外）都有"lab3"和"YOUR CODE"的注释，请在提交时特别注意保持注释，并将"YOUR CODE"替换为自己的学号，并把所有标有对应注释的部分填上正确的代码。
