# Lab 3 提示词汇总

> **本文件的组织方式**
>
> 本文件按小组成员分部分汇总本实验使用过的提示词：
>
> - **部分一：Trap 上下文恢复与 Trap 入口** —— 负责人 2413628-黄乐仁
> - **部分二：中断与异常处理** —— 负责人 2410868-孙锘梵
> - **部分三：收口与验证** —— 负责人 2412796-杨斯源（组长）
>
> 统一采用 lab0.5 规定的四段式框架：
>
> ```
> [PROMPT]        做什么、改哪里、怎么改
> [RELY]          可以依赖什么——宏定义、数据结构、已有函数
> [GUARANTEE]     必须实现或修改什么——需要交付的函数列表
> [SPECIFICATION] 每个函数的详细行为规格（Pre-Condition / Post-Condition / Case / Requirements）
> ```
>
> **lab3 的特点**：本实验依赖 lab2，代码中有两处 `//lab2 请补充你在lab2中的代码`，需要**用你此前设计的提示词重新生成 lab2 的代码并填回**（见各部分的「提示词 0」）。此外两个功能模块都是汇编/底层处理，给 AI 的 `[RELY]` 必须包含**完整的 `SAVE_ALL` 与 `struct trapframe` 布局**——这是本实验最容易出错的地方。

---

## 一、部分一：Trap 上下文恢复与 Trap 入口

### 提示词 0：补回 lab2 的 `page_init()`

> 沿用 lab2 使用过的 `page_init` 提示词（见 lab2 分支的 `report/prompt.md` 第一节），
> 把生成结果填入 `kern/mm/pmm.c` 中标有 `//lab2 请补充你在lab2中的代码` 的位置。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：实现 `RESTORE_ALL` 与 `__alltraps`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

**第一轮：生成 `RESTORE_ALL` 与 `__alltraps`。** 生成后编译通过，但 `make grade` 中 Test 1 始终为 0/35。

**定位过程：**

1. 先排除 `RESTORE_ALL` 本身——在 `__trapret` 下 GDB 断点，核对栈上保存的 32 个通用寄存器与 CSR，数值全部正确（`t0=0x105`、`epc` 已推进）；单步走完恢复序列，`t0` 也确实恢复成了 `0x105`。
2. 继续在 `ebreak` 返回后的**第一条指令**处下断点，发现该地址上的实际指令是 `addi a2, sp, 36`，而源码对应行是 `li a0, 0x105`——说明**返回地址错了**。

**根因：** 断点异常里硬编码了 `tf->epc += 4`（注释还写着"ebreak 是 4 字节指令"），但内核开启了 C 扩展（`rv64imafdch`），`ebreak` 被汇编成 **16 位的 `c.ebreak`**。`epc += 4` 于是落到了下一条 4 字节指令 `li a0,261` 的**正中间**，CPU 从那里取指解出垃圾指令，`a0` 从未被赋成期望值，第一条 `bne` 必然失败。

```asm
0x...1a00:  9002        ebreak        ← 2 字节（c.ebreak）
0x...1a02:  10500513    li a0,261     ← 4 字节，占 0x1a02~0x1a05
0x...1a06:  0ca29863    bne t0,a0,...
```

**解决：** 与非法指令异常一致，**按实际指令长度推进** `epc`——读 `epc` 处的 16 位字，低两位为 `0b11` 表示 32 位指令（+4），否则为 16 位（+2）。修复后 Test 1 通过（35/35）。

> 这个问题虽然**表现在部分一的测试上**，但根因在部分二的 `exception_handler` 里；收口阶段由组长统一定位并修复。

---

## 二、部分二：中断与异常处理

### 提示词 0：补回 lab2 的 Best-Fit 模块

````markdown
[PROMPT]
**任务**：修改 `kern/mm/best_fit_pmm.c`，补回最佳适配（Best-Fit）物理内存管理模块：初始化空闲区、把连续物理页注册为空闲块、分配时从所有足够大的块中挑出最小的一块、释放时合并物理相邻的块，并注册为 `best_fit_pmm_manager` 供框架统一调用。

**操作要求**：必须在项目中修改实际文件。已提供的 `basic_check()` 与 `best_fit_check()` 原样保留，不得修改或重复实现；新增代码只插入 `//lab2 请补充你在lab2中的代码` 标记处与文件末尾。

**输出要求**：使用下方信息，以工程真实定义为准，不引入与物理内存管理无关的功能。

[RELY]
```c
// memlayout.h
typedef struct { list_entry_t free_list; unsigned int nr_free; } free_area_t;
struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; };

// 宏与函数
PG_property / PG_reserved
SetPageProperty(p) / ClearPageProperty(p) / PageProperty(p)
PageReserved(p) / set_page_ref(p, n) / page_ref(p)
le2page(le, page_link) / page2pa(p)
npage / pages / PGSIZE / nr_free_pages()
```

[GUARANTEE]
```c
static void best_fit_init(void);
static void best_fit_init_memmap(struct Page *base, size_t n);
static struct Page *best_fit_alloc_pages(size_t n);
static void best_fit_free_pages(struct Page *base, size_t n);
static size_t best_fit_nr_free_pages(void);

const struct pmm_manager best_fit_pmm_manager;   // 头文件已有 extern 声明，不可加 static
```

[SPECIFICATION]
## best_fit_init
**Post-Condition**：空闲链表为空，`nr_free` 为 0。

## best_fit_init_memmap
**Pre-Condition**：`[base, base+n)` 为连续物理页，已置 `PG_reserved`，`n > 0`。
**Post-Condition**：每页 `flags`/`property` 清零、引用计数为 0；仅首页 `property = n` 并置 `PG_property`；`nr_free` 增加 `n`；按物理地址升序插入链表。
**Case 1** 链表为空：直接挂上。
**Case 2** 链表非空：插到第一个地址更大的节点之前，没有则追加到表尾。

## best_fit_alloc_pages
**Pre-Condition**：`n > 0`。
**Post-Condition**：返回的块在所有长度不小于 `n` 的空闲块中**最小**；清除返回页的 `PG_property`；`nr_free` 减少 `n`；块有剩余时从第 `n` 页起成为新块首页并留在链表中，地址顺序不变。
**Case 1** `n` 大于 `nr_free`：返回 `NULL`，不改动结构。
**Case 2** 无足够大的块：返回 `NULL`。
**Case 3** 恰好等长：摘除节点，整块返回。
**Case 4** 块更大：摘除原节点，把剩余部分插回原位。

## best_fit_free_pages
**Pre-Condition**：`[base, base+n)` 未置 `PG_reserved`、未置 `PG_property`，`n > 0`。
**Post-Condition**：页面 `flags` 清零、引用计数为 0；首页 `property = n` 并置 `PG_property`、按地址升序插入；与相邻空闲块物理连续时合并，被并块从链表摘除；`nr_free` 增加 `n`。
**Case 1** 仅与前块邻接：并入前块。
**Case 2** 仅与后块邻接：后块并入本块。
**Case 3** 两侧都邻接：三者合成一块。

## best_fit_nr_free_pages
**Post-Condition**：返回 `nr_free`。

**Requirements**：
- 链表始终按物理地址升序，否则相邻块无法识别、合并失效。
- `property` 仅在空闲块首页有意义，分配与释放中必须及时置位或清除。
- 挑「最小可用块」而非「第一个可用块」——这是与 First-Fit 的唯一语义差别。
- 不得修改两个测试函数，不得改动 `struct pmm_manager` 的字段。
- 不要求修改 `pmm.c` 中的管理器选择逻辑。
````

### 提示词 1：实现 `interrupt_handler()`

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

### 提示词 2：实现 `exception_handler()`

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
- 打印 64 位地址时使用 64 位格式符，避免截断。
````

**生成的代码出了什么问题、怎么解决的：**

**提示词 0（Best-Fit）**

1. **整段覆盖会让测试函数重复定义。** lab2 阶段的 `best_fit_pmm.c` 自带 `basic_check()` 与 `best_fit_check()`，本实验框架又提供了一个同名版本，若直接整文件覆盖就是两份同名定义，在 `-Werror` 下编译必然失败。解决：在 `[PROMPT]` 的操作要求里明确「两个测试函数原样保留、不得重复实现」，并限定新增代码只能落在 `//lab2` 标记处与文件末尾。

**提示词 1、提示词 2（两个 handler）**

2. **`%d` 与 64 位实参不匹配。** 未知中断需要输出中断号的十进制值，而 `cause` 是 64 位 `uintptr_t`；变参函数不做类型提升，直接把 64 位值交给 `%d` 属于未定义行为。解决：输出前显式转换为 `int`，并在 `[SPECIFICATION]` 中要求「输出的中断号取剥离标志位之后的值」。

3. **兜底分支不能解引用 `tf->epc`。** 非法指令分支需要读回触发异常的指令来判断长度，但若把读取放在函数入口统一执行，遇到 `epc` 为空的人工构造 `trapframe` 就会访存失败。解决：在 `[SPECIFICATION]` 的 `Requirements` 中写明「只有非法指令分支读取 `tf->epc` 处的内存」。

4. **`-Werror` 下的声明顺序。** 两个 handler 若写在 `trap_dispatch()` 之后，会因隐式声明直接编译失败。解决：在 `[PROMPT]` 的操作要求中限定「新增代码写入标记处且位于 `trap_dispatch()` 之前」。

5. **误加 `static` 会破坏外部调用。** 这两个函数会被本文件之外的代码直接调用，加上 `static` 会导致链接失败。解决：`[GUARANTEE]` 中逐个标注「不得为 static」，并在 `[PROMPT]` 中重申。

6. **关机要求与框架不一致。** 指导书建议统计输出累计达到 `SHUTDOWN_AFTER` 后调用关机接口，但框架并未定义该阈值宏；而过早关机会让时钟计数失去继续增长的观测窗口。解决：`Case 1` 只要求「续期 → 计数 → 输出」的闭环，不加入关机判断。

---

## 三、部分三：收口与验证

本部分负责报告汇总、格式校对与最终验证，**没有代码生成任务**，因此没有对应的提示词。

本部分的工作内容见 `docs/lab3-tasks.md` 第三节。

---

## 四、迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | 部分一 / 部分二 · 提示词 0（补回 lab2 代码） | 补回后内核仍完全无法启动，QEMU 只打印 OpenSBI banner、没有一行内核输出 | 定位到 `entry.S` 的页表只映射了第 511 项（高 1G），缺低 1G 恒等映射；补第 2 项后正常启动 |
| 2 | 部分一 · 提示词 1（`RESTORE_ALL` / `__alltraps`） | Test 1 始终 0/35；但 GDB 验证 `RESTORE_ALL` 本身恢复的寄存器是正确的 | 继续定位到"返回地址错误"，把排查方向转到断点异常的 `epc` 推进长度 |
| 3 | 部分二 · 提示词 2（`exception_handler`） | 断点异常硬编码 `tf->epc += 4`；`c.ebreak` 实际只有 2 字节，返回地址落在 4 字节指令中间，执行出垃圾指令 | 改为按指令长度判定：读 `epc` 处 16 位字，低两位 `0b11` → +4，否则 +2；修复后 100/100 |
