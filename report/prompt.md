# Lab 4 提示词汇总

> **本文件的组织方式**
>
> 本文件按小组成员分部分汇总本实验使用过的提示词：
>
> - **部分一：虚拟内存页操作** —— 负责人 2413628-黄乐仁
> - **部分二：进程创建及初始化** —— 负责人 2410868-孙锘梵
> - **部分三：进程调度** —— 负责人 2412796-杨斯源（组长）
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
> **lab4 的特点**：本实验依赖 lab2 与 lab3，代码里有四处 `//lab2` / `//lab3` 注释需要**用此前设计的提示词重新生成并填回**（见各部分的「提示词 0」「提示词 1」）。补回时注意：lab4 的物理内存管理器虽然叫 `default_pmm_manager`，内部仍是 Best-Fit，测试函数名也仍是 `best_fit_check()`。

---

## 一、部分一：虚拟内存页操作

### 提示词 0：补回 lab2 的 `page_init()`

> 沿用 lab2 使用过的 `page_init` 提示词（见 lab2 分支的 `report/prompt.md`），
> 把生成结果填入 `kern/mm/pmm.c` 中标有 `//lab2 请补充你在lab2中的代码` 的位置。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：补回 lab3 的 `RESTORE_ALL` 与 `__alltraps`

> 沿用 lab3 的提示词（见 lab3 分支的 `report/prompt.md` 第一节「提示词 1」），
> 填入 `kern/trap/trapentry.S` 的两处 `#lab3:填写你在lab3中实现的代码`。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 2：实现 `page_insert()` 与 `page_remove()`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

> **待填写（负责人：黄乐仁）**

---

## 二、部分二：进程创建及初始化

### 提示词 0：补回 lab2 的 Best-Fit 管理模块

> 沿用 lab2 的 Best-Fit 提示词，填入 `kern/mm/default_pmm.c` 的 `//lab2 请补充你在lab2中的代码`。
> 注意管理器结构体名要改成 `default_pmm_manager`；`basic_check()` / `best_fit_check()` 由框架提供，不得重复实现。

````markdown
[PROMPT]

**任务**：修改 `code/kern/mm/default_pmm.c`，在 `//lab2 请补充你在lab2中的代码` 标记处补回 lab2 实现的 Best-Fit 物理内存管理器，包括 `best_fit_init`、`best_fit_init_memmap`、`best_fit_alloc_pages`、`best_fit_free_pages`、`best_fit_nr_free_pages` 五个函数，并在文件末尾定义 `const struct pmm_manager default_pmm_manager` 把它们注册进去。

**操作要求**：你必须在项目中修改实际文件，而不是只展示代码。只在标记处插入代码，保留文件已有的 include、注释和两个框架测试函数。lab4 骨架已经把 `basic_check()` 和 `best_fit_check()` 写好，**不得修改，也不得重复实现**；lab4 没有 `best_fit_pmm.h`，全部代码只写进 `default_pmm.c` 这一个文件。

**输出要求**：直接进行文件操作，使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION] 的信息，以工程真实代码为准，不要引入 lab2 之外的功能（如 SLUB、伙伴系统）。

[RELY]

```c
// 宏定义（kern/mm/memlayout.h，default_pmm.c 已经 include <pmm.h>）
#define PGSIZE 4096                  // 页大小
#define PG_reserved 0                // flags 第 0 位：保留页
#define PG_property 1                // flags 第 1 位：空闲块首页
SetPageReserved(page) / ClearPageReserved(page) / PageReserved(page)
SetPageProperty(page) / ClearPageProperty(page) / PageProperty(page)
#define le2page(le, member) to_struct((le), struct Page, member)

// 数据结构
struct Page {
    int ref;                      // 页引用计数
    uint_t flags;                 // 状态标志位
    unsigned int property;        // 空闲块页数，仅块首页有效
    list_entry_t page_link;       // 空闲链表节点
};
typedef struct {
    list_entry_t free_list;       // 空闲块链表头
    unsigned int nr_free;         // 空闲页总数
} free_area_t;

struct pmm_manager {
    const char *name;
    void (*init)(void);
    void (*init_memmap)(struct Page *base, size_t n);
    struct Page *(*alloc_pages)(size_t n);
    void (*free_pages)(struct Page *base, size_t n);
    size_t (*nr_free_pages)(void);
    void (*check)(void);
};

// 可用函数（libs/list.h、kern/mm/pmm.h）
list_init(list_entry_t *elm);
list_add(list_entry_t *elm, list_entry_t *prev);      // 插到 prev 之后
list_add_before(list_entry_t *elm, list_entry_t *next); // 插到 next 之前
list_del(list_entry_t *elm);
list_empty(list_entry_t *list);
list_next(list_entry_t *list);
page2pa(struct Page *page);  pa2page(uintptr_t pa);  page2kva(struct Page *page);
```

[GUARANTEE]

必须实现或修改的接口：
```c
static void best_fit_init(void);
static void best_fit_init_memmap(struct Page *base, size_t n);
static struct Page *best_fit_alloc_pages(size_t n);
static void best_fit_free_pages(struct Page *base, size_t n);
static size_t best_fit_nr_free_pages(void);

// 文件末尾，供 kern/mm/pmm.c 的 pmm_manager = &default_pmm_manager 使用
const struct pmm_manager default_pmm_manager;
```

你可以根据需要添加：static 辅助函数、模块内部宏（如 `free_list` / `nr_free`）、`static free_area_t` 变量。

[SPECIFICATION]

## 模块内部状态
必须有一个 `static free_area_t free_area;`，并用 `#define free_list (free_area.free_list)`、`#define nr_free (free_area.nr_free)` 简化访问。

## best_fit_init
**Pre-Condition**：
- 系统刚进入 `pmm_init`，尚未注册任何空闲物理页。

**Post-Condition**：
- 空闲块链表为空，空闲页计数为 0。

**Case 1**：
- 无参数。把 `free_list` 初始化为空链表（头结点的前驱和后继都指向自己），并置 `nr_free = 0`。

## best_fit_init_memmap
**Pre-Condition**：
- `base` 指向连续 `n` 个物理页的第一个描述符，`n > 0`；这些页当前被标记为保留（`PageReserved` 为真），不属于任何空闲块。

**Post-Condition**：
- 这 `n` 页整体构成一个空闲块，块首页 `property == n` 且 `PG_property` 置位；该块按物理地址升序插入 `free_list`；`nr_free` 增加 `n`；块内所有页引用计数为 0、`PG_property` 清零（只有块首页置位）。

**Case 1（正常初始化）**：
- 逐页清除标志位与引用计数、初始化 `page_link`，然后以块首页身份登记 `property = n`，再遍历 `free_list` 按地址顺序插入。

**Requirements**：
- 插入后链表必须保持物理地址升序，后续分配与合并都依赖这个不变量。
- 不得调用 `best_fit_free_pages` 完成初始化（此时这些页还标记为保留，free 路径会断言失败）。

## best_fit_alloc_pages
**Pre-Condition**：
- 已通过 `best_fit_init_memmap` 注册空闲页；`n > 0`。

**Post-Condition**：
- 成功时返回一个恰好 `n` 页的空闲块首页，块内页不再属于空闲块，`nr_free` 减少 `n`；失败时返回 `NULL`，空闲区状态不变。

**Case 1（`n > nr_free`）**：
- 直接返回 `NULL`。

**Case 2（找到合适的块）**：
- 遍历 `free_list`，在 `property >= n` 的块中选 `property` **最小**的一个（Best-Fit；`property` 相同时取链表中靠前、即物理地址较低者）。
- 从该块**头部**切出 `n` 页作为分配结果；剩余 `property - n` 页若大于 0，则成为新的块首页并按原位置插回链表；剩余为 0 时把原块首页摘除。
- 被分配的块首页清除 `PG_property`，块内所有页引用计数清零，`nr_free -= n`，返回切出的块首页。

**Case 3（没有满足条件的块）**：
- 返回 `NULL`，链表与 `nr_free` 均不变。

**Requirements**：
- 必须是真正的最小适配：不能退化成找到第一个 `property >= n` 就返回。
- 返回值是 `struct Page *`，必须指向分配块的首页，不能返回块内中间页。

## best_fit_free_pages
**Pre-Condition**：
- `base` 指向此前由 `best_fit_alloc_pages` 分配出去的连续 `n` 页，`n > 0`；这些页当前 `PG_property` 为 0。

**Post-Condition**：
- 这 `n` 页重新成为空闲块并挂回 `free_list`；与物理相邻的空闲块自动合并；`nr_free` 增加 `n`；链表仍按地址升序。

**Case 1（与前一块相邻）**：
- 若前一块首页加上它的 `property` 正好等于 `base`，则把当前块并入前一块（前一块 `property += n`），不新增链表节点。

**Case 2（与后一块相邻）**：
- 若 `base + n` 正好是后一块的首页，则把后一块并入当前块（当前块 `property += 后一块->property`），并删除后一块的链表节点。

**Case 3（两侧都相邻）**：
- 先并前块，再并后块，三个块合成一个。

**Case 4（两侧都不相邻）**：
- 当前块作为独立节点按地址升序插入 `free_list`。

**Requirements**：
- 释放前逐页清零标志与引用计数，块首页重新置 `PG_property`。
- 合并只能用 `property` 和页地址判断，且要在插入链表前完成，避免先插入再合并导致节点重复。

## best_fit_nr_free_pages
**Pre-Condition**：
- 无。

**Post-Condition**：
- 返回当前空闲页总数，不修改任何状态。

**Case 1**：
- 直接返回 `nr_free`。

## default_pmm_manager
**Pre-Condition**：
- 上述五个函数均已定义。

**Post-Condition**：
- 文件末尾存在 `const struct pmm_manager default_pmm_manager`，`.name` 为 `"default_pmm_manager"`，五个函数指针指向上述实现，`.check` 指向框架已有的 `best_fit_check`。

**Requirements**：
- 结构体变量名必须是 `default_pmm_manager`（`default_pmm.h` 里是 `extern const struct pmm_manager default_pmm_manager;`，`pmm.c` 直接取地址），内部函数名保持 `best_fit_*`。
- 只允许注册已有的 `best_fit_check`，不要改写或重复定义检查函数。
````

### 提示词 1：补回 lab3 的 `interrupt_handler()` 与 `exception_handler()`

> 沿用 lab3 的提示词（见 lab3 分支 `report/prompt.md` 第二节「提示词 1」「提示词 2」），
> 填入 `kern/trap/trap.c` 的 `//lab3 请补充你在lab3中的代码`。

````markdown
[PROMPT]

**任务**：修改 `code/kern/trap/trap.c`，在 `//lab3 请补充你在lab3中的代码` 标记处补回 lab3 实现的两个 Trap 处理函数：`interrupt_handler()`（中断类 Trap）与 `exception_handler()`（异常类 Trap）。

**操作要求**：你必须在项目中修改实际文件，而不是只展示代码。只在该标记处插入两个函数，保留文件已有的 include、`print_ticks()`、`print_trapframe()`、`print_regs()` 等代码。lab4 的 `trap()` 已经写成 `if ((intptr_t)tf->cause < 0) interrupt_handler(tf); else exception_handler(tf);`（位于文件末尾），因此两个函数**必须定义在 `trap()` 之前**。lab4 删掉了 lab3 的 `trap_dispatch()` 与 `trap_in_kernel()`，不要补回来；也不要实现 `do_pgfault()`，缺页处理不属于本任务。

**输出要求**：直接进行文件操作，使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION] 的信息，以工程真实代码为准。

[RELY]

```c
// 数据结构（kern/trap/trap.h）
struct pushregs {
    uintptr_t zero, ra, sp, gp, tp, t0, t1, t2,
              s0, s1, a0, a1, a2, a3, a4, a5, a6, a7,
              s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
              t3, t4, t5, t6;
};
struct trapframe {
    struct pushregs gpr;
    uintptr_t status;
    uintptr_t epc;        // 触发 trap 的指令地址
    uintptr_t badvaddr;
    uintptr_t cause;      // scause：最高位 1 = 中断，0 = 异常
};

// 常量（libs/riscv.h）
#define IRQ_S_SOFT   1
#define IRQ_S_TIMER  5
#define IRQ_S_EXT    9
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT          0x3

// 本文件已有 / 已声明的可用符号
#define TICK_NUM 100
static void print_ticks(void);           // 每 TICK_NUM 次打印一次；DEBUG_GRADE 下会 panic("EOT")
void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *gpr);
extern volatile size_t ticks;            // kern/driver/clock.h
void clock_set_next_event(void);         // 装载下一次时钟中断
int cprintf(const char *fmt, ...);
```

[GUARANTEE]

必须实现或修改的接口：
```c
void interrupt_handler(struct trapframe *tf);
void exception_handler(struct trapframe *tf);
```

你可以根据需要添加 static 辅助函数或宏，但不要改变上述两个函数的签名，也不要新增全局函数。

[SPECIFICATION]

## interrupt_handler
**Pre-Condition**：
- `tf->cause` 最高位为 1，其余位是中断号；进入时 `__alltraps` 已保存完整现场。

**Post-Condition**：
- 对应中断被处理；定时器中断推进系统时钟并在每 100 次打印一次；未知中断打印现场后返回。

**Case 1（`IRQ_S_TIMER` 时钟中断）**：
- 调用 `clock_set_next_event()` 装载下一次时钟；`ticks` 加一；当 `ticks % TICK_NUM == 0` 时调用 `print_ticks()`。

**Case 2（`IRQ_S_SOFT` 软件中断）**：
- 打印 `Supervisor software interrupt`。

**Case 3（`IRQ_S_EXT` 外部中断）**：
- 打印 `Supervisor external interrupt`。

**Case 4（其他中断号）**：
- 打印中断号并调用 `print_trapframe(tf)`。

**Requirements**：
- 必须先用 `tf->cause & ~(1UL << 63)` 剥离最高位得到中断号，再进入 `switch`；直接对含符号的 `cause` 比较会走错分支。
- 每个分支处理完要 `break` 或 `return`，不能让定时器中断继续落入默认分支，否则每次时钟都会多打印一条 Unknown。
- 定时器分支要先 `clock_set_next_event()` 再累加 `ticks`，顺序反了会让时钟停摆。

## exception_handler
**Pre-Condition**：
- `tf->cause` 最高位为 0，`tf->cause` 即异常号；`tf->epc` 是触发异常的指令地址。

**Post-Condition**：
- 异常被处理；对于可恢复异常，`tf->epc` 已指向**下一条**指令，返回后不会重复触发同一异常。

**Case 1（`CAUSE_ILLEGAL_INSTRUCTION` 非法指令）**：
- 打印 `Illegal instruction at 0x%016lx`（地址取 `tf->epc`）；读取 `tf->epc` 处的 16 位指令，若低 2 位为 `0x3` 说明是 32 位指令，`tf->epc += 4`，否则是 16 位压缩指令，`tf->epc += 2`。

**Case 2（`CAUSE_BREAKPOINT` 断点）**：
- 打印 `Breakpoint at 0x%016lx`；按与 Case 1 相同的规则推进 `tf->epc`。

**Case 3（其他异常）**：
- 打印异常号并调用 `print_trapframe(tf)`。

**Requirements**：
- 推进 `epc` 必须按指令长度区分 2/4 字节：固定加 4 会让压缩指令跳到错误位置，不加则会反复触发同一异常形成死循环。
- 读取指令时假定 `tf->epc` 指向内核代码段，可直接按 `uint16_t *` 访问。
````

### 提示词 2：实现进程创建与初始化

> 涉及 `alloc_proc` / `proc_init` / `kernel_thread` / `do_fork` / `copy_thread` / `copy_mm` 六个函数（`kern/process/proc.c` 的 `//LAB4 function2 YOUR CODE`）。

````markdown
[PROMPT]

**任务**：修改 `code/kern/process/proc.c`，在 `//LAB4 function2 YOUR CODE` 标记处实现进程创建与初始化模块，包括 `alloc_proc`、`proc_init`、`kernel_thread`、`do_fork`、`copy_thread`、`copy_mm` 六个函数。

**操作要求**：你必须在项目中修改实际文件，而不是只展示代码。只在标记处补全这六个函数，保留文件已有代码：`get_pid`、`forkret`、`hash_proc`、`find_proc`、`setup_kstack`、`put_kstack`、`do_exit`、`init_main` 已经给出，直接调用，不要重写。同文件末尾的 `//LAB4 function3 YOUR CODE` 属于进程调度部分，不要改动。

**输出要求**：直接进行文件操作，使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION] 的信息，以工程真实代码为准，不要引入用户进程、文件系统等本实验之外的功能。

[RELY]

```c
// 数据结构（kern/process/proc.h）
enum proc_state {
    PROC_UNINIT = 0,   // 未初始化
    PROC_SLEEPING,
    PROC_RUNNABLE,     // 可运行（就绪或在跑）
    PROC_ZOMBIE,
};
struct context {          // switch_to 保存/恢复的寄存器
    uintptr_t ra;  uintptr_t sp;
    uintptr_t s0;  uintptr_t s1;  uintptr_t s2;  uintptr_t s3;
    uintptr_t s4;  uintptr_t s5;  uintptr_t s6;  uintptr_t s7;
    uintptr_t s8;  uintptr_t s9;  uintptr_t s10; uintptr_t s11;
};
#define PROC_NAME_LEN 15
#define MAX_PROCESS 4096
#define MAX_PID (MAX_PROCESS * 2)

struct proc_struct {
    enum proc_state state;         // 进程状态
    int pid;                       // 进程 ID
    int runs;                      // 已运行次数
    uintptr_t kstack;              // 内核栈基址
    volatile bool need_resched;    // 是否需要被换下 CPU
    struct proc_struct *parent;    // 父进程
    struct mm_struct *mm;          // 内存管理信息（本阶段为 NULL）
    struct context context;        // 切换上下文
    struct trapframe *tf;          // 中断帧
    uintptr_t pgdir;               // 页目录基址（物理地址）
    uint32_t flags;                // 进程标志
    char name[PROC_NAME_LEN + 1];  // 进程名
    list_entry_t list_link;        // 进程链表节点
    list_entry_t hash_link;        // 进程哈希表节点
};

// trapframe（kern/trap/trap.h）：gpr 含 zero/ra/sp/.../s0/s1/a0/...，
// 其后是 status、epc、badvaddr、cause。

// 常量与全局变量
extern char bootstack[], bootstacktop[];   // kern/init/entry.S 的启动栈
#define KSTACKPAGE 2                        // 内核栈页数
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)    // 内核栈大小 = 8KB
#define HASH_SHIFT 10
#define HASH_LIST_SIZE (1 << HASH_SHIFT)   // = 1024
extern list_entry_t proc_list;
extern struct proc_struct *idleproc, *initproc, *current;
static list_entry_t hash_list[HASH_LIST_SIZE];
static int nr_process;                      // 已用 PCB 数
extern uintptr_t boot_pgdir_pa;             // 页目录物理地址（kern/mm/pmm.h）

// 寄存器常量（libs/riscv.h）
#define SSTATUS_SIE  0x00000002
#define SSTATUS_SPIE 0x00000020
#define SSTATUS_SPP  0x00000100
read_csr(sstatus) / write_csr(sstatus, val) / set_csr(sstatus, bit)

// 仅内核线程相关：entry.S 的 kernel_thread_entry 依次执行
//   move a0, s1;  jalr s0;  jal do_exit
// 即 s0 = 入口函数、s1 = 参数；forkrets(tf) 会走 __trapret → RESTORE_ALL → sret。

// 文件内已给出的辅助函数
static int get_pid(void);                       // 从 1 开始循环分配 pid
static void forkret(void);                      // forkrets(current->tf)
static void hash_proc(struct proc_struct *p);
static struct proc_struct *find_proc(int pid);
static int setup_kstack(struct proc_struct *p);// alloc_pages(KSTACKPAGE)，成功返回 0
static void put_kstack(struct proc_struct *p);
void wakeup_proc(struct proc_struct *proc);     // sched.c，置 PROC_RUNNABLE
char *set_proc_name(struct proc_struct *p, const char *name);
void *kmalloc(size_t n);  void kfree(void *objp);
#define local_intr_save(x)   // 关中断并把原状态存入 x
#define local_intr_restore(x)// 恢复中断状态
// 错误码（libs/error.h）：E_NO_MEM、E_NO_FREE_PROC，使用时取负
```

[GUARANTEE]

必须实现或修改的接口：
```c
static struct proc_struct *alloc_proc(void);
static int copy_mm(uint32_t clone_flags, struct proc_struct *proc);
static void copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf);
int kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags);
void proc_init(void);
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);
```

你可以根据需要添加 static 辅助函数或模块内部宏，但不要改变上述签名，也不要新增全局变量。

[SPECIFICATION]

## alloc_proc
**Pre-Condition**：
- `kmalloc` 已初始化可用。

**Post-Condition**：
- 成功时返回一个所有字段均为确定初值的 PCB；失败时返回 `NULL`。

**Case 1（kmalloc 失败）**：
- 返回 `NULL`，不修改任何全局状态。

**Case 2（分配成功）**：
- 整块清零后设置：`state = PROC_UNINIT`、`pid = -1`、`pgdir = boot_pgdir_pa`、`need_resched = 0`；其余字段（`parent`、`mm`、`tf`、`runs`、`flags`、`kstack`、`context`、`name`、两个链表节点）保持零值。

**Requirements**：
- 必须整体清零而不是逐字段挑着赋值：测试会断言 `tf == NULL`、`parent == NULL`、`mm == NULL`、`runs == 0`、`flags == 0`、`context.ra == 0`。
- `pgdir` 存的是页目录**物理地址**（`proc_run` 要写进 satp），不能存虚拟地址。

## proc_init
**Pre-Condition**：
- 内核已进入 `kern_init`；`kmalloc_init`、`pmm_init` 已完成；`current` 尚无效。

**Post-Condition**：
- `proc_list` 与 `hash_list` 的 1024 个桶全部初始化；`idleproc` 为 pid 0、`PROC_RUNNABLE`、`kstack == bootstack`、`need_resched == 1`、名字 `"idle"`；`current == idleproc`；`initproc` 为 pid 1、父进程 `idleproc`、`PROC_RUNNABLE`、名字 `"init"`；`nr_process == 2`。

**Case 1（创建 idleproc）**：
- 分配 PCB 并设置上述字段，`nr_process` 置 1，把 `current` 指向它。

**Case 2（创建 initproc）**：
- 调用 `kernel_thread(init_main, "Hello world!!", 0)` 创建；返回 pid 小于等于 0 时 `panic`；随后用 `find_proc(pid)` 取回 PCB 并命名为 `"init"`。

**Requirements**：
- `idleproc` **不能**通过 `kernel_thread`/`do_fork` 创建：它直接复用启动栈，`tf` 与 `context.ra` 必须保持 0（测试断言 `idleproc->tf == NULL`、`context.ra == 0`）。
- `current = idleproc` 必须在调用 `kernel_thread` **之前**完成，否则 `do_fork` 里 `proc->parent = current` 会把 initproc 的父进程设错。
- `nr_process` 要在创建 idleproc 后置 1，之后由 `do_fork` 自增。
- 测试会在 `proc_init` 返回后检查：`current == idleproc`、`idleproc->pid == 0`、`idleproc->kstack == (uintptr_t)bootstack`、`idleproc->state == PROC_RUNNABLE`、`initproc->pid == 1`、`initproc->parent == idleproc`、`initproc->state == PROC_RUNNABLE`。

## kernel_thread
**Pre-Condition**：
- `kernel_thread_entry` 已链接进内核；`clone_flags` 由调用者给出（如 0）。

**Post-Condition**：
- 成功时新建一个内核线程，处于 `PROC_RUNNABLE`；它第一次被调度时从 `fn(arg)` 开始执行，运行在 S 态且开中断。

**Case 1（do_fork 成功）**：
- 返回新线程的 pid（大于 0）。

**Case 2（do_fork 失败）**：
- 返回负错误码，不残留 PCB 或内核栈。

**Requirements**：
- 必须先构造一个全零的 `struct trapframe` 局部变量，再填：`gpr.s0 = (uintptr_t)fn`、`gpr.s1 = (uintptr_t)arg`、`epc = (uintptr_t)kernel_thread_entry`。这与 `kernel_thread_entry` 的 `move a0, s1; jalr s0` 严格对应，寄存器不能换。
- `status` 要置 `SSTATUS_SPP`（sret 后回到 S 态）与 `SSTATUS_SPIE`（sret 后开中断），并清 `SSTATUS_SIE`（sret 前保持关中断）。
- 不要直接调用 `fn(arg)`，必须通过 `do_fork` 走调度器。

## copy_thread
**Pre-Condition**：
- `proc` 已分配好内核栈 `kstack`；`tf` 是调用者提供的 trapframe 模板；`esp` 为期望的用户栈顶（内核线程为 0）。

**Post-Condition**：
- `proc->tf` 指向其内核栈顶部的 trapframe 副本；`proc` 首次被 `switch_to` 后，经 `forkret → __trapret → sret` 跳到 `tf->epc` 开始执行。

**Case 1（`esp != 0`，指定了栈）**：
- `gpr.sp = esp`。

**Case 2（`esp == 0`，内核线程）**：
- `gpr.sp = (uintptr_t)proc->tf`，即让它从自己的内核栈顶开始压栈。

**Requirements**：
- `proc->tf` 必须放在 `kstack + KSTACKSIZE` 之下（栈顶往下第一个 trapframe 的位置），不能越过栈顶。
- `context.ra` 必须是 `forkret`，不能写成 `kernel_thread_entry`：入口地址是通过 `sret` 从 `tf->epc` 跳过去的，`ra` 只负责第一次 `switch_to` 的返回点。
- `context.sp` 指向 `proc->tf`；`gpr.a0` 置 0，作为子线程的“返回值”。
- `esp == 0` 时必须回退到内核栈顶：若直接令 `gpr.sp = 0`，线程第一次压栈就会访问非法地址。

## copy_mm
**Pre-Condition**：
- `current` 指向当前进程。

**Post-Condition**：
- 新进程的 `mm` 字段被正确设置；本实验所有进程都是内核线程。

**Case 1（`current->mm == NULL`）**：
- 置 `proc->mm = NULL`，返回 0。

**Case 2（`current->mm != NULL`，用户进程）**：
- `panic`，lab4 不支持用户进程地址空间复制。

**Requirements**：
- 保持该签名与调用位置，便于 lab5 扩展；不要复制页表，也不要修改 `pgdir`。

## do_fork
**Pre-Condition**：
- `current` 指向当前进程；`clone_flags`、`stack`、`tf` 由调用者给出。

**Post-Condition**：
- 成功时新 PCB 已挂入 `proc_list` 和哈希表，`nr_process` 加一，状态为 `PROC_RUNNABLE`，返回其 pid；失败时返回负错误码，且不残留 PCB 或内核栈。

**Case 1（`nr_process >= MAX_PROCESS`）**：
- 返回 `-E_NO_FREE_PROC`。

**Case 2（`alloc_proc` 失败）**：
- 返回 `-E_NO_MEM`。

**Case 3（`setup_kstack` 失败）**：
- 释放 PCB 后返回 `-E_NO_MEM`。

**Case 4（`copy_mm` 失败）**：
- 释放内核栈和 PCB 后返回相应错误码。

**Case 5（全部成功）**：
- 设 `proc->parent = current`，调用 `copy_thread`，然后在关中断临界区内完成 `pid = get_pid()`、`hash_proc(proc)`、`list_add(&proc_list, &proc->list_link)`、`nr_process++`；退出临界区后 `wakeup_proc(proc)`，返回 `proc->pid`。

**Requirements**：
- pid 分配、入哈希表、入进程链表必须在同一个 `local_intr_save` 临界区内完成，否则并发 fork 可能拿到重复 pid。
- 错误路径按“内核栈 → PCB”的逆序回滚，不能漏也不能重复释放。
- 返回前新进程必须已置为 `PROC_RUNNABLE`（`wakeup_proc`），否则调度器永远不会选中它。
````

**生成的代码出了什么问题、怎么解决的：**

两个 handler 的逻辑与 lab3 相同，问题都出在「lab4 的骨架已经变了」：

1. **照搬 lab3 会多出两个没人调用的函数。** lab3 的 `trap.c` 有 `trap_dispatch()` 和 `trap_in_kernel()`，lab4 把分流直接写进了 `trap()`，这两个函数已不存在。提示词限定只补 `interrupt_handler()` 与 `exception_handler()`，其余一概不动。
2. **插入位置决定能否编译。** lab4 的 `trap()` 在文件末尾，并且它直接调用两个 handler。若按 lab3 的位置写在 `trap()` 之后，基线构建就会报 `kern/trap/trap.c:102: implicit declaration of function 'interrupt_handler'`（`exception_handler` 同理，第 107 行）。提示词的 `[PROMPT]` 因此明确要求「写在中部标记处、位于 `trap()` 之前」，而不是泛泛地说「补在文件里」。
3. **`epc` 的推进量是 lab3 踩过的坑。** lab3 第一版按「`ebreak` 是 4 字节」写死 `epc += 4`，但内核开了 C 扩展，`ebreak` 被汇编成 2 字节的 `c.ebreak`，返回地址落到下一条 4 字节指令的正中间，取指解出垃圾指令。提示词把「读 `epc` 处 16 位字、低两位为 `0b11` 才是 4 字节」写进 `[SPECIFICATION]` 的 Case 与 Requirements，两个异常分支共用同一套判断。

最后生成结果与 lab3 的实现一致，只是去掉了 lab4 不需要的两个辅助函数。

---

## 三、部分三：进程调度

### 提示词 0：实现调度与上下文切换

> 涉及 `cpu_idle` / `schedule` / `proc_run` / `switch_to`（`proc.c`、`sched.c`、`switch.S` 的 `//LAB4 function3 YOUR CODE`）。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

> **待填写（负责人：杨斯源）**

---

## 四、迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | | | |
