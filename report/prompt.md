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
> 把生成结果填入 `kern/mm/pmm.c` 中标有 `//lab2 function1 YOUR CODE` 的位置，并把 `YOUR CODE` 替换为学号 `2413628`。
> lab3 与 lab2 的 `pmm.c` 在物理内存管理上完全一致，因此这条提示词直接复用、无需修改规格。

````markdown
[PROMPT]
**任务**：实现 `kern/mm/pmm.c` 中标有 `//lab2 function1 YOUR CODE` 位置的 `static void page_init(void)`，完成物理内存探测与页描述符初始化，并把注释里的 `YOUR CODE` 替换为我的学号 `2413628`（`//lab2` 注释必须保留）。

**操作要求**：只修改 `kern/mm/pmm.c` 这一个文件；不得改动 `pmm_init()` 等已有函数；实现后必须在 `-Werror` 下编译通过。

**输出要求**：直接给出可编译的代码实现。

[RELY]
```c
// ---- 地址常量（kern/mm/memlayout.h）----
#define PHYSICAL_MEMORY_OFFSET  0xFFFFFFFF40000000   // 虚地址 - 物理地址
#define PHYSICAL_MEMORY_END     0x88000000

// ---- 页与页号（kern/mm/mmu.h / libs/defs.h）----
#define PGSIZE   4096
#define PGSHIFT  12
#define PPN(la)  (((uintptr_t)(la)) >> PGSHIFT)
#define ROUNDUP(a, n) / ROUNDDOWN(a, n)

// ---- 全局符号（kern/mm/pmm.c / pmm.h）----
extern struct Page *pages;       // 页描述符数组（待初始化）
extern size_t npage;             // 可管理物理页总数（待初始化）
extern const size_t nbase;       // = DRAM_BASE / PGSIZE = 0x80000
extern uint64_t va_pa_offset;    // 虚实映射偏移（待初始化）
uint64_t get_memory_base(void);  // kern/driver/dtb.h，设备树解析出的内存起始地址
uint64_t get_memory_size(void);  // 设备树解析出的内存大小

// ---- 页描述符（kern/mm/memlayout.h）----
struct Page { int ref; uint64_t flags; unsigned int property; list_entry_t page_link; };
#define SetPageReserved(page)  ((page)->flags |= (1UL << 0))   // PG_reserved

// ---- 地址转换（kern/mm/pmm.h）----
#define PADDR(kva) ...                            // 虚拟地址 → 物理地址（依赖 va_pa_offset）
static inline struct Page *pa2page(uintptr_t pa); // 物理地址 → Page 描述符
static inline uintptr_t page2pa(struct Page *p);  // Page → 物理地址

// ---- 交给管理器的入口（kern/mm/pmm.c）----
static void init_memmap(struct Page *base, size_t n);  // 内部调用 pmm_manager->init_memmap

// ---- 链接符号 ----
extern char end[];   // 内核镜像结束位置（虚拟地址）
```

[GUARANTEE]
```c
static void page_init(void);   // kern/mm/pmm.c，把 //lab2 function1 YOUR CODE 替换为 2413628
```

[SPECIFICATION]
## page_init
**Pre-Condition**：`init_pmm_manager()` 已调用（`pmm_manager` 已选定）；`dtb_init()` 已调用（`get_memory_base()/get_memory_size()` 已就绪）。
**Post-Condition**：`va_pa_offset`、`npage`、`pages` 已正确设置；内核镜像与页描述符数组所占页均处于 `PageReserved` 状态；其余空闲页已通过 `init_memmap()` 交给管理器。

**Requirements（按顺序）**：
1. 设 `va_pa_offset = PHYSICAL_MEMORY_OFFSET`；
2. 取 `mem_base = get_memory_base()`、`mem_size = get_memory_size()`，任一为 0 则 `panic` 终止；
3. 设 `npage = PPN(mem_base + mem_size)`，并保证 `npage > nbase`；
4. 设 `pages = (struct Page *)ROUNDUP((uintptr_t)end, PGSIZE)`（放在内核 `end` 之后、按页对齐）；
5. 把下标 `[0, npage - nbase)` 范围内的所有 Page 描述符先全部 `SetPageReserved`；
6. 计算首个可分配字节 `freemem = PADDR((uintptr_t)pages + sizeof(struct Page) * npage)`；
7. 对区间 `[ROUNDUP(freemem, PGSIZE), ROUNDDOWN(mem_end, PGSIZE))`（若非空）调用 `init_memmap(pa2page(begin), (end_pa - begin) / PGSIZE)`；**没有任何完整空闲页时不得调用**。

## 验收（与 lab2 的 `tools/test_lab2.c` 8 条一致）
1. `va_pa_offset == PHYSICAL_MEMORY_OFFSET`
2. `get_memory_base()` / `get_memory_size()` 均非 0
3. `npage == PPN(mem_base + mem_size)` 且 `npage > nbase`
4. `pages` 非空、按 `PGSIZE` 对齐、位于 `ROUNDUP(end, PGSIZE)` 之后
5. `nr_free_pages()` ∈ `(0, npage - nbase]`
6. `PageReserved(pa2page(PADDR(end)))` 为真（内核镜像尾页保留）
7. `PageReserved(pa2page(PADDR(pages)))` 为真（页描述符数组保留）
8. `pa2page(page2pa(q)) == q`（往返一致）

## Requirements（全局）
- `-Werror` 下无警告；内核无 libc，不得调用 `malloc`/`printf` 等标准库；
- 注意区分**虚拟地址 / 物理地址 / 页号 / Page 指针**四者；调用 `PADDR`/`pa2page` 之前必须已设好 `va_pa_offset` 与 `pages`。
````

**补回说明（2413628-黄乐仁）：**

lab3 的 `page_init()` 与 lab2 完全一致，直接用 lab2 的提示词重新生成并填回即可。填回时重点核对两处 lab2 阶段踩过的坑：

1. **地址类型混淆**：`end` 是内核**虚拟地址**，`freemem` 是「pages 数组末尾」的虚拟地址，与物理的 `mem_end` 比较前都必须先 `PADDR` 转换；`pages` 则要先 `ROUNDUP((uintptr_t)end, PGSIZE)` 按页对齐。
2. **遗漏保留区**：内核镜像占用的页和**页描述符数组自身占用的页**都要 `PageReserved`。实现里用「先把 `[0, npage - nbase)` 全部 `SetPageReserved`，再从 `freemem` 之后释放空闲页」的方式一次性覆盖两块保留区。

验证：填回后内核能正常启动（`make grade` 不再因缺 `page_init` 而启动失败），后续 Trap 测试在此基础上全部通过。

### 提示词 1：实现 `RESTORE_ALL` 与 `__alltraps`

````markdown
[PROMPT]
**任务**：修改 `kern/trap/trapentry.S`，在 `//lab3 function1 YOUR CODE` 处补全 `RESTORE_ALL` 宏，并在 `__alltraps` 中建立与 C 函数 `trap(struct trapframe *tf)` 的调用关系，把 `YOUR CODE` 替换为我的学号 `2413628`。

**操作要求**：必须在项目中修改实际文件；只补 `RESTORE_ALL` 与 `__alltraps` 两处，`SAVE_ALL`、`__trapret`、`struct trapframe`、`trap()` 的函数签名一律不动；必须在 `-Werror` 下编译通过。

**输出要求**：使用下方信息，以工程真实定义为准。

[RELY]
```asm
/* 寄存器宽度：rv64 下 REGBYTES = 8，STORE = sd，LOAD = ld */

/* struct trapframe 布局（kern/trap/trap.h），栈上共 36 个 slot × 8 字节：
     gpr[0..31]  // struct pushregs，32 个通用寄存器（下标 = 寄存器号）
     status      // slot 32 —— sstatus
     epc         // slot 33 —— sepc
     badvaddr    // slot 34 —— stval
     cause       // slot 35 —— scause */

/* 已提供的 SAVE_ALL（不允许修改，RESTORE_ALL 必须与之严格对称）： */
.macro SAVE_ALL
    csrw sscratch, sp           // 先把被打断前的 sp 暂存到 sscratch
    addi sp, sp, -36 * REGBYTES // 在内核栈上开 36 个 slot 的 trapframe
    STORE x0,  0*REGBYTES(sp)
    STORE x1,  1*REGBYTES(sp)
    STORE x3,  3*REGBYTES(sp)
    ...                         // x4..x31 依次存到对应下标（slot 2 留空，见下）
    STORE x31, 31*REGBYTES(sp)
    csrrw s0, sscratch, x0      // 取回原 sp，同时把 sscratch 清 0（标记“来自内核”）
    csrr s1, sstatus
    csrr s2, sepc
    csrr s3, stval
    csrr s4, scause
    STORE s0, 2*REGBYTES(sp)    // 原 sp 存到 slot 2
    STORE s1, 32*REGBYTES(sp)   // sstatus → slot 32
    STORE s2, 33*REGBYTES(sp)   // sepc    → slot 33
    STORE s3, 34*REGBYTES(sp)   // stval   → slot 34
    STORE s4, 35*REGBYTES(sp)   // scause  → slot 35
.endm
```

```
/* RISC-V 调用约定：a0 是第一个参数寄存器；jal 把返回地址写回 ra（x1）。
   trap() 声明为 void trap(struct trapframe *tf)，入参 tf 就是 trapframe 指针。 */
/* 返回：sret 会把 sepc 写回 pc，所以 epc 是否被 handler 修正，决定了返回后从哪条指令继续。 */
```

[GUARANTEE]
```asm
RESTORE_ALL            // .macro，恢复被打断现场
__alltraps             // 入口，SAVE_ALL 之后把 tf 传给 trap()
```

[SPECIFICATION]
## RESTORE_ALL
**Pre-Condition**：`sp` 指向当前内核栈上的完整 trapframe（36 slot，由 `SAVE_ALL` 建立）。
**Post-Condition**：除 `x0`、`sp` 外的通用寄存器恢复到 trapframe 中保存的值；`sstatus`/`sepc` 已恢复；`sp` 恢复为被打断前的值。

**Requirements**：
- 恢复顺序必须与 `SAVE_ALL` 的保存布局一一对应（下标 = 槽位号）。
- `x0` 硬连线为 0，槽位 0 不必也不应被 `LOAD` 覆盖。
- `sp`（槽位 2）必须**最后**恢复：恢复其他寄存器时要继续用 `sp` 去寻址栈上数据，一旦提前改写 `sp`，后续所有 `LOAD` 都读错位置。
- 先恢复 `sepc` 与 `sstatus` 没有副作用；`sret` 会读取它们，因此恢复完直接 `sret` 即可回到被打断处。
- `x1`/`ra` 无需特殊处理：`SAVE_ALL` 已把被打断代码的返回地址存进槽位 1，`RESTORE_ALL` 用 `LOAD x1, 1*REGBYTES(sp)` 恢复即可，期间被 `jal trap` 改写的 `ra` 会被这次 `LOAD` 覆盖。
- `sscratch` 无需在 `RESTORE_ALL` 中恢复：`SAVE_ALL` 已把它清 0（用作「本次 Trap 来自内核」的标记），下一次 Trap 进入时 `SAVE_ALL` 会重新写入，旧值无需还原。

## __alltraps
**Pre-Condition**：硬件按 `stvec` 跳转至此；`SAVE_ALL` 已执行完毕，`sp` 指向 trapframe。
**Post-Condition**：`trap()` 已被调用且入参为 trapframe 指针；从 `trap()` 返回后 `sp` 仍指向同一 trapframe（供后续 `RESTORE_ALL` 使用）。

**Requirements**：
- 用 `mv a0, sp` 把 trapframe 指针放入参数寄存器 `a0`，再 `jal trap`。
- 不得在 `jal trap` 之前破坏 `sp`；`trap()` 按调用约定保存/恢复被调用者寄存器，`sp` 由调用者保证不变。
- 调用结束后落入 `__trapret`（`RESTORE_ALL` + `sret`）。
- `jal trap` 会改写 `ra`，但被打断代码的 `ra` 已被 `SAVE_ALL` 存入 trapframe 槽位 1，因此此处覆盖 `ra` 是安全的，`RESTORE_ALL` 会从栈上恢复它。
- `__alltraps` 入口的 `.align(2)` 保证 4 字节对齐，满足 `stvec` 低两位必须为 0（直接模式）的要求。
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
