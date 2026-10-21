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

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

[待填写]

---

## 二、部分二：页面分配算法

### 提示词 1：实现 Best-Fit 物理内存管理模块（`kern/mm/best_fit_pmm.c`）

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

[待填写]

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

测试代码一次通过编译。但要注意：`buddy_check()` 的运行时验证**依赖
`page_init()` 把空闲区交给 `init_memmap()`**，因此在部分一完成之前只能做
静态编译验证，无法真正跑起来。这一点已在分工文档 §0.5 中说明。

---

## 四、迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | | | |
| 2 | | | |
| 3 | | | |
