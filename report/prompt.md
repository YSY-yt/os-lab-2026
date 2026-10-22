# Lab 2 提示词汇总

> **本文件的组织方式**
>
> 本文件按小组成员分部分汇总本实验使用过的提示词：
>
> - **部分一：物理内存初始化** —— 负责人 2413628-黄乐仁
> - **部分二：页面分配算法（First-Fit / Best-Fit）** —— 负责人 2410868-孙锘梵
> - **部分三：伙伴系统与收口** —— 负责人 2412796-杨斯源（组长）
>
> 各部分提示词由对应负责人提供，统一采用 lab0.5 规定的四段式框架：
>
> ```
> [PROMPT]        做什么、改哪里、怎么改
> [RELY]          可以依赖什么——宏定义、数据结构、已有函数
> [GUARANTEE]     必须实现或修改什么——需要交付的函数列表
> [SPECIFICATION] 每个函数的详细行为规格（Pre-Condition / Post-Condition / Case / Requirements）
> ```
>
> **lab2 与 lab1 的区别**：lab1 是阅读与调试类实验，不生成代码；lab2 是**真正要让 AI 生成代码**的，因此本文件里的提示词是实打实的实现规格。课程还要求报告里说明「**使用提示词让 AI 生成代码的过程中出现了什么 bug，以及你是怎么解决的**」，这部分记录请一并写进下面的「迭代记录」。

---

## 一、部分一：物理内存初始化

### 提示词 1：实现 `page_init()`（`kern/mm/pmm.c`）

````markdown
[PROMPT]
**任务**：实现 `kern/mm/pmm.c` 中标有 `//lab2 function1 YOUR CODE` 位置的 `static void page_init(void)`，完成物理内存探测与页描述符初始化，并把注释里的 `YOUR CODE` 替换为我的学号 2413628（`//lab2` 注释必须保留）。

**操作要求**：只修改 `kern/mm/pmm.c` 这一个文件；不得改动其它任何文件，也不得改动 `pmm_init()` 等已有函数。实现后必须在 `-Werror` 下编译通过，并满足下方 8 条验收标准。

**输出要求**：直接给出可编译的代码实现。

[RELY]
// ---- 地址常量（kern/mm/memlayout.h）----
#define PHYSICAL_MEMORY_OFFSET  0xFFFFFFFF40000000   // 虚地址 - 物理地址
#define KERNEL_BEGIN_PADDR      0x80200000
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

// ---- 调试 ----
panic(fmt, ...); assert(x);

// ---- 链接符号 ----
extern char end[];   // 内核镜像结束位置（虚拟地址）

[GUARANTEE]
必须交付：`kern/mm/pmm.c` 中 `static void page_init(void)` 的完整实现，并把注释里的 `YOUR CODE` 替换为 `2413628`。

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

## 验收（与 tools/test_lab2.c 的 8 条一致）
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

**生成的代码出了什么问题、怎么解决的：**

**第一轮：生成初版实现。** 让 AI 参考经典 ucore `page_init()` 生成代码。初版照搬 x86 版思路，用 `e820map` 结构探测内存，与本实验的 RISC-V 移植不一致——本实验没有 `e820map`，物理内存范围来自设备树（`dtb_init()` 解析后由 `get_memory_base()/get_memory_size()` 提供），且虚实映射偏移是 `PHYSICAL_MEMORY_OFFSET` 而非 x86 的 `KERNBASE` 关系。

**第二轮：改成设备树方案并逐条对照 8 条验收。** 修正后让 AI 重新按 RISC-V 平台的常量实现：先设 `va_pa_offset`，再从设备树取 `mem_base/mem_size` 算 `npage`，把 `pages` 放到内核 `end` 之后按页对齐。此过程中重点核对了两处容易出错的地方：

1. **地址类型混淆**：`end` 是内核**虚拟地址**，直接当物理地址用会错；必须 `PADDR(end)` 或 `ROUNDUP((uintptr_t)end, PGSIZE)` 得到虚拟地址后再 `PADDR` 转物理。`freemem` 也是同理——它是"pages 数组末尾"的虚拟地址，必须 `PADDR` 之后才能与物理的 `mem_end` 做比较。
2. **遗漏保留区**：不能只保留内核镜像占用的页，**页描述符数组自身占用的页也必须 `PageReserved`**，否则数组所在页会被当成空闲页交给管理器，破坏描述符本身。实现里用"先全部 `SetPageReserved`，再从 `freemem` 之后释放"的方式一次性覆盖了内核镜像与描述符数组两块保留区。

**第三轮：编译 + 验收收敛。** 修正后在 `-Werror` 下编译通过、零警告；逐条核对 `tools/test_lab2.c` 的 8 条断言（`va_pa_offset`、`npage`、`pages` 对齐、空闲页数区间、内核尾页与数组页均 reserved、`page2pa`/`pa2page` 往返），并确认 `make grade` 中 `[Test 1] Page Init: PASS`。

---

## 二、部分二：页面分配算法

### 提示词 1：空壳构建（`kern/mm/best_fit_pmm.c`）

````markdown
[PROMPT]
改kern/mm/best_fit_pmm.c,参照default_pmm.c补齐Best-Fit模块骨架让工程能编译。文件现在只有basic_check和best_fit_check两个测试函数,本阶段先按First-Fit实现。保留现有代码,只在//lab2标记处往下补。

[RELY]
free_area_t、Page结构体及页标志宏见kern/mm/memlayout.h,list_*见libs/list.h,set_page_ref和pmm_manager接口见kern/mm/pmm.h,best_fit_pmm.h已声明best_fit_pmm_manager。

[GUARANTEE]
实现best_fit_init/init_memmap/alloc_pages/free_pages/nr_free_pages,注册best_fit_pmm_manager(.check=best_fit_check),可加free_area和free_list/nr_free宏。行为同default_pmm.c对应函数。

[SPECIFICATION]
````

### 提示词 2：规划分配算法修改

````markdown
[PROMPT]
分析default_pmm.c(First-Fit)与Best-Fit的差异,给出best_fit_alloc_pages的修改规划,本次不改代码只输出规划。

[RELY]
best_fit_pmm.c的best_fit_alloc_pages现在是First-Fit:遇到第一个property>=n就返回。两算法共用同一数据结构(地址升序链表+块首页property记大小),区别只在选哪个块。

[GUARANTEE]
输出:唯一要改的函数、要改的代码段(查找循环)、改后选择逻辑、哪些函数不用改及原因。结论是只改best_fit_alloc_pages的查找循环,从取第一个够用块改为遍历整条链表取property最小的够用块,其余函数不改。

[SPECIFICATION]
````

### 提示词 3：执行修改

````markdown
[PROMPT]
根据刚才的分析改kern/mm/best_fit_pmm.c,把best_fit_alloc_pages从First-Fit改成Best-Fit。只改best_fit_alloc_pages,其他函数和两个测试函数不动。

[RELY]
当前best_fit_alloc_pages是First-Fit:循环里遇到第一个property>=n就break。Best-Fit要求遍历整条free_list,在property>=n的块里选property最小的。

[GUARANTEE]
只改best_fit_alloc_pages的查找循环,切分、nr_free更新、清PG_property等逻辑不变。

[SPECIFICATION]
遍历整条链表选property>=n且最小者,摘除、够大切剩余块插回、nr_free减n、清块头PG_property后返回;n>nr_free或无满足块返NULL。
````
**生成的代码出了什么问题、怎么解决的：**

初始`best_fit_pmm.c` 只有两个测试函数，
`free_list` / `nr_free` 未声明导致编译失败。参照 `default_pmm.c` 补齐
`free_area`、两个宏、五个管理函数与 `best_fit_pmm_manager` 注册结构，
此时分配逻辑仍是 First-Fit，整棵树已能编译。

---

## 三、部分三：伙伴系统与收口

### 提示词 1：从零生成 Buddy System 模块并接入内核

````markdown
[PROMPT]
**任务**：在 `kern/mm/` 下新建 `buddy_pmm.c` 与 `buddy_pmm.h`，实现一个完整的 Buddy System（伙伴系统）连续物理内存分配器，并按 `struct pmm_manager` 的接口注册为 `buddy_pmm_manager`。

**操作要求**：你必须在项目中创建真实文件，而不是仅展示代码。保留现有代码，不要改动已有文件；`pmm.c` 里只允许在验证阶段由人工临时切换管理器，本提示词不要求你修改它。

**输出要求**：直接进行文件操作，使用下面 `[RELY]`、`[GUARANTEE]`、`[SPECIFICATION]` 部分的信息。

[RELY]
// ---- 页描述符（kern/mm/memlayout.h）----
struct Page {
    int ref;                // 引用计数
    uint64_t flags;         // bit0 = PG_reserved, bit1 = PG_property
    unsigned int property;  // 空闲块头页用它记录块大小/阶数
    list_entry_t page_link; // 空闲链表节点
};
#define PG_reserved 0
#define PG_property 1
#define SetPageReserved(page)       ((page)->flags |= (1UL << PG_reserved))
#define ClearPageReserved(page)     ((page)->flags &= ~(1UL << PG_reserved))
#define PageReserved(page)          (((page)->flags >> PG_reserved) & 1)
#define SetPageProperty(page)       ((page)->flags |= (1UL << PG_property))
#define ClearPageProperty(page)     ((page)->flags &= ~(1UL << PG_property))
#define PageProperty(page)          (((page)->flags >> PG_property) & 1)

typedef struct {
    list_entry_t free_list;   // 空闲链表头
    unsigned int nr_free;     // 该链表上的空闲页数
} free_area_t;

// ---- 地址与页号换算（kern/mm/pmm.h、mmu.h）----
#define PGSIZE   4096
#define PGSHIFT  12
#define PPN(la)  (((uintptr_t)(la)) >> PGSHIFT)
static inline ppn_t      page2ppn(struct Page *page);   // page - pages + nbase
static inline uintptr_t  page2pa(struct Page *page);    // page2ppn(page) << PGSHIFT
static inline struct Page *pa2page(uintptr_t pa);
#define PADDR(kva) ...     // 虚拟地址 → 物理地址
extern struct Page *pages;
extern size_t npage;
extern const size_t nbase;          // = DRAM_BASE / PGSIZE = 0x80000
extern uint64_t va_pa_offset;

// ---- 管理器接口（kern/mm/pmm.h）----
struct pmm_manager {
    const char *name;
    void (*init)(void);
    void (*init_memmap)(struct Page *base, size_t n);
    struct Page *(*alloc_pages)(size_t n);
    void (*free_pages)(struct Page *base, size_t n);
    size_t (*nr_free_pages)(void);
    void (*check)(void);
};

// ---- 链表 API（libs/list.h）----
list_init(list_entry_t *elm);
list_empty(const list_entry_t *list);
list_add(list_entry_t *elm, list_entry_t *prev);         // 插到 prev 之后
list_add_before(list_entry_t *elm, list_entry_t *next);  // 插到 next 之前
list_del(list_entry_t *listelm);
list_next / list_prev
#define le2page(le, member)  to_struct((le), struct Page, member)

// ---- 调试与输出 ----
assert(x);  panic(fmt, ...);  cprintf(fmt, ...);

// ---- 容量依据 ----
// 可管理物理内存 128 MiB ÷ 4 KiB = 32768 页 = 2^15
// 故最大块为 order 15，MAX_ORDER 取 16（合法 order 0..15）

[GUARANTEE]
必须交付：

`kern/mm/buddy_pmm.h`
```c
extern const struct pmm_manager buddy_pmm_manager;
```

`kern/mm/buddy_pmm.c`
```c
static void          buddy_init(void);
static void          buddy_init_memmap(struct Page *base, size_t n);
static struct Page  *buddy_alloc_pages(size_t n);
static void          buddy_free_pages(struct Page *base, size_t n);
static size_t        buddy_nr_free_pages(void);
static void          buddy_check(void);

const struct pmm_manager buddy_pmm_manager = { ... };
```

你可以根据需要添加：static 辅助函数（求阶、对齐分解、合并等）、内部宏、模块内静态变量（如 `free_area[]`、`nr_free`）。

[SPECIFICATION]
## 数据结构与约定
**Pre-Condition**：无。
**Post-Condition**：模块内部状态满足下列约定。
**Requirements**：
- 用 `free_area_t free_area[MAX_ORDER]` 为**每个阶**维护一条空闲链表，`free_area[k]` 存放所有 2^k 页空闲块的**头页**；
- 空闲块头页置 `PG_property`，且 `property = order`；
- 已分配块清 `PG_property`，但 `property` **仍保留该块的 order**，供 `free_pages` 还原块大小；
- 维护全局空闲页数 `nr_free`，**单位是页，不是块**；
- `MAX_ORDER = 16`（合法 order 0..15）。

## buddy_init
**Pre-Condition**：尚未初始化。
**Post-Condition**：所有阶的空闲链表为初始空状态，`nr_free = 0`。
**Requirements**：不得访问任何物理页。

## buddy_init_memmap
**Pre-Condition**：`base` 指向一段连续物理页，这些页当前处于 `PageReserved` 状态；`n > 0`；**本函数可能被多次调用**（对应多段空闲区）。
**Post-Condition**：`[base, base+n)` 全部登记为可分配；`nr_free` 增加 n；空闲链表按阶组织完毕。
**Case 1（区域起点已对齐到 2^k 且长度足够）**：以该阶直接登记。
**Case 2（区域起点未对齐，或长度不是 2 的幂）**：必须把区域**拆成若干"起点对齐、大小为 2^k"的块**逐块登记。
**Case 3（n == 0）**：不做任何登记。
**Requirements**：
- 逐页 `assert(PageReserved(p))`，随后清 `flags`、`ref` 置 0；
- 只有**块头页**置 `PG_property` 并写 `property = order`；
- 登记后每块头页的下标必须满足 `(index % (1 << order)) == 0` —— **这是后续伙伴地址计算正确的前提，必须满足**；
- 拆块时取"既不超过剩余页数、又能整除起点下标"的最大阶。

## buddy_alloc_pages
**Pre-Condition**：已调用 `buddy_init` 与至少一次 `buddy_init_memmap`。
**Post-Condition**：返回一个 order 为 k 的块的头页，满足 `2^k >= n`；该块已从空闲链表摘除；`nr_free` 减去 `2^k`。
**Case 1（n == 0）**：返回 `NULL`。
**Case 2（所需阶超过当前最大可用块）**：返回 `NULL`。
**Case 3（存在同阶空闲块）**：直接摘除并返回。
**Case 4（只有更大的块）**：逐级**对半分裂**，每次把一半插回低一阶的空闲链表，直到得到所需阶。
**Requirements**：
- `k = ceil(log2(n))`；若 `k >= MAX_ORDER` 返回 `NULL`；
- 分裂后插回的块必须保持空闲块约定（头页置 `PG_property`、`property = order`）；
- 返回前清该块头页的 `PG_property`，并写 `property = k`。

## buddy_free_pages
**Pre-Condition**：`base` 是此前由 `buddy_alloc_pages` 返回的块头页。
**Post-Condition**：该块回到空闲链表；`nr_free` 增加该块的页数；若伙伴块空闲则已完成合并。
**Case 1（伙伴不空闲或不同阶）**：直接把该块插入对应阶链表。
**Case 2（伙伴空闲且同阶）**：把两块合并为高一阶的块，并对新块**继续尝试合并**，直到不能再合并。
**Requirements**：
- **块大小必须从 `base->property` 取出，不得用参数 n 反推**（调用方传入的 n 可能小于实际块大小）；
- 伙伴下标：`buddy_index = index ^ (1 << order)`，其中 `index = base - pages`；
- 合并后的头页取两块中**地址较小者**；
- 合并必须是**循环（或递归）**的，不能只做一层。

## buddy_nr_free_pages
**Post-Condition**：返回当前空闲页总数（页，不是块）。

## buddy_check
**Post-Condition**：运行模块自研测试。本提示词阶段可先实现为调用一个空的测试函数并打印提示，测试内容由下一条提示词单独设计。

## Requirements（全局）
- 编译必须在 `-Werror` 下**无警告**；
- 内核无 libc：不得调用 `malloc` / `printf` 等标准库，只能用 `assert` / `panic` / `cprintf`；
- 不得修改 `pmm.c` 的 `init_pmm_manager()` 以外的内容（管理器切换由人工在验证时临时完成，最终提交会切回 Best-Fit）。
````

### 提示词 2：为 Buddy System 设计测试函数

````markdown
[PROMPT]
**任务**：针对上一步生成的 Buddy System 实现（`kern/mm/buddy_pmm.c`），设计一套测试并接入 `buddy_check()`，验证分配、释放与合并的正确性。

**操作要求**：你必须在项目中修改真实文件（`kern/mm/buddy_pmm.c`），保留现有实现，只增加测试代码；不得修改 `pmm_manager` 接口，也不得为了"让测试通过"而改动 `buddy_*` 的实现。

**输出要求**：直接进行文件操作。测试要能**独立判断成败**（用 `assert`），并在结束时**恢复状态**，不影响后续使用。

[RELY]
- 已实现并可调用：`buddy_init` / `buddy_init_memmap` / `buddy_alloc_pages` / `buddy_free_pages` / `buddy_nr_free_pages`
- 框架提供的可观测接口：`alloc_page()`、`free_page()`、`alloc_pages(n)`、`free_pages(base, n)`、`nr_free_pages()`
- 空闲链表 `free_area[k]`（每阶一条）：头页满足 `PageProperty(p)` 且 `p->property == k`
- 调试输出：`assert` / `cprintf`

[GUARANTEE]
必须交付：
- `static void buddy_check(void);` 内的完整测试逻辑
- 可按需增加 static 辅助函数（如"统计某阶块数""校验全局一致性"）
- 覆盖下列**全部** 10 个核心行为与 5 个边界场景

[SPECIFICATION]
## 前置条件
**Pre-Condition**：`buddy_init()` 已调用、`buddy_init_memmap()` 已登记至少一段空闲区（由 `pmm_init()` 的调用链保证）。
**Post-Condition**：所有断言通过；测试结束时空闲页数恢复到初始值。

## 必须覆盖的核心行为
**Case 1（初始一致性）**：各阶链表 `nr_free` 之和 == `nr_free_pages()`；链表上每个头页 `PageProperty` 为真且 `property == 阶`。
**Case 2（单页分配）**：`alloc_page()` 非空；`nr_free_pages()` 恰好减少 1。
**Case 3（2 的幂分配）**：分配 2 / 4 / 8 页均成功，`nr_free_pages()` 各减少对应页数。
**Case 4（非 2 的幂请求）**：分配 3 页成功，但 `nr_free_pages()` 减少 **4**（内部向上取整到 4）。
**Case 5（分裂）**：大块空闲时申请小块能成功，且分裂出的剩余块仍在空闲链表中。
**Case 6（释放不合并）**：只释放一块（其伙伴仍被占用），`nr_free_pages()` 增加对应页数且**不发生**合并。
**Case 7（伙伴合并）**：释放一对伙伴后，两者合并为更高一阶的块。
**Case 8（递归向上合并）**：连续分配 8 页后全部释放，应一路合并回 order 3 的块。
**Case 9（耗尽）**：持续分配直到返回 `NULL`，此时 `nr_free_pages() == 0`。
**Case 10（不泄漏）**：反复"分配 N 页 → 释放 N 页"若干轮，每轮结束 `nr_free_pages()` 都回到初始值。

## 必须覆盖的边界
**Case 11**：`alloc_pages(0)` 返回 `NULL`。
**Case 12**：请求超过当前最大空闲块时返回 `NULL`。
**Case 13**：请求超过 `2^(MAX_ORDER-1)` 页时返回 `NULL`。
**Case 14**：释放后立即按同大小再分配，能成功。
**Case 15**：分配两个块后，**先释放低地址再释放高地址**与**反序释放**，两种顺序结果一致且都能正确合并。

## Requirements
- 每个 Case 用 `assert` 判定，失败即 `panic`；关键步骤用 `cprintf` 打点，便于定位；
- 测试**不得修改**被测模块的约定（不得为通过测试而改 `buddy_*` 的实现）；
- **每个 Case 结束时必须把状态恢复到该 Case 开始时的值**（分配出去的都要释放），不得跨 Case 污染；
- 只用 `assert` / `cprintf`，不得引入标准库；
- 全部通过时打印一行汇总，便于截图存证。
````

**生成的代码出了什么问题、怎么解决的：**

**第一轮（提示词一 → 生成模块）**

第一次生成后，`buddy_pmm.c` 在 `-Werror` 下编译报错：

```
kern/mm/buddy_pmm.c:220:42: error: initialization of 'struct Page *' from
'size_t' {aka 'long long unsigned int'} makes pointer from integer
without a cast [-Wint-conversion]
  220 |   struct Page *buddy = pages + bi;
```

**原因**：不变量检查函数里声明了一个局部变量 `unsigned int pages`，把全局的
`struct Page *pages`（页描述符数组）**遮蔽**了，导致 `pages + bi` 从"指针
+ 偏移"退化成整数加法。

**解决**：把该局部变量改名为 `pgs`，编译通过，`-Werror` 下无警告。

> 这一点提醒我们：内核里 `pages`、`npage`、`nbase` 都是全局符号，写局部变量
> 时很容易撞名，而编译器在 `-Werror` 下会直接拦下来——这也是为什么要在
> `[RELY]` 里把这些全局符号明确列给 AI。

**第二轮（提示词二 → 生成测试）**

15 个用例写好后在 QEMU 中真正跑起来，**Case 3 断言失败**：

```
kernel panic at kern/mm/buddy_pmm.c:266:
    assertion failed: nr_free_pages() == before
```

**原因**：释放时与伙伴合并，只在**该阶链表**的计数里减去了伙伴的页数，
**没有同步减全局 `nr_free`**；而合并结束后又按**合并后的整块大小**累加到
`nr_free`，于是伙伴的页数被重复计算了一次。

**解决**：在摘除伙伴时同步执行 `nr_free -= ORDER_PAGES(k)`。修复后
**15 个用例全部通过**（`ALL 15 CASES PASSED`）。

> 这个 bug 很隐蔽：它不崩溃、不报错，只会让 `nr_free` 缓慢偏大，肉眼几乎
> 看不出来——正是靠"分配后释放，`nr_free` 必须回到原值"这条断言才抓住。

---

## 四、迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | 部分一 · `page_init`（第一轮） | AI 按 x86 的 `e820map` 思路实现内存探测，与本实验 RISC-V 的设备树路径不符 | 在 `[RELY]` 中补充真实接口 `get_memory_base()` / `get_memory_size()`，并明确映射偏移是 `PHYSICAL_MEMORY_OFFSET` |
| 2 | 部分一 · `page_init`（第二轮） | 地址类型混淆：`end` 是**虚拟地址**，不能直接与物理地址比较 | 在 `[SPECIFICATION]` 中要求区分虚拟地址/物理地址/物理页号，规定先用 `PADDR()` 转换再比较 |
| 3 | 部分三 · 提示词一（生成 Buddy 模块） | 局部变量 `unsigned int pages` 遮蔽全局 `struct Page *pages`，指针加法退化为整数加法（`-Wint-conversion`） | 在 `[RELY]` 中明确列出 `pages` / `npage` / `nbase` 等全局符号；代码中改名为 `pgs` |
| 4 | 部分三 · 提示词二（生成测试） | 合并伙伴时未同步递减全局 `nr_free`，导致伙伴页数被重复计算，Case 3 的 `nr_free_pages() == before` 断言失败 | 在 `[SPECIFICATION]` 中补上"摘除伙伴时同步递减全局计数"；测试中增加"页数守恒"不变量 |
