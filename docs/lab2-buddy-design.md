# lab2 部分三：Buddy System 设计思路

> 负责人：2412796-杨斯源（组长）
> 对应练习：功能模块二 · 任务三
> 说明：本文件是**动手前的设计稿**，用于先想清楚结构、算法与验证方案，再据此写两份 prompt 让 AI 生成代码。

---

## 一、目标与约束

### 1.1 任务要求

| 项 | 要求 |
|---|---|
| 模块 | 从零实现 Buddy System 连续物理内存分配器，并接入内核 |
| 框架 | **实验不提供任何代码框架**（没有 `buddy_pmm.c`、没有 `default_pmm.c` 那样的范本可抄） |
| 测试 | **实验不提供测试函数**，需自己设计 |
| prompt | 交**两份**：① 生成模块 ② 针对该模块设计测试 |
| 复杂度 | 时空复杂度不做要求，**能通过测试即可** |

### 1.2 与框架的契约

必须实现 `struct pmm_manager` 的六个成员（见 `kern/mm/pmm.h`）：

```c
struct pmm_manager {
    const char *name;
    void       (*init)(void);
    void       (*init_memmap)(struct Page *base, size_t n);
    struct Page *(*alloc_pages)(size_t n);
    void       (*free_pages)(struct Page *base, size_t n);
    size_t     (*nr_free_pages)(void);
    void       (*check)(void);
};
```

调用链：`pmm_init() → init_pmm_manager() → page_init() → init_memmap() → alloc_pages()/free_pages()`。

**必须遵守的语义：**

- `init_memmap(base, n)`：把 `[base, base+n)` 这段连续物理页登记为可分配。**可能被调用多次**（`page_init()` 会按空闲区逐段调用）。
- `alloc_pages(n)`：分配 **≥ n 页连续物理内存**，返回**首页**的 `struct Page *`；失败返回 `NULL`。
- `free_pages(base, n)`：释放从 `base` 起的 n 页。
- `nr_free_pages()`：返回**当前空闲页总数**（以页为单位，不是块数）。

---

## 二、数据结构设计

### 2.1 可用的 Page 字段

```c
struct Page {
    int ref;                // 引用计数
    uint64_t flags;         // PG_reserved(bit0) / PG_property(bit1)
    unsigned int property;  // 空闲块的头页用它记录块大小/阶数
    list_entry_t page_link; // 空闲链表节点
};
```

### 2.2 本模块的约定

| 约定 | 说明 |
|---|---|
| **以阶（order）描述块** | 块大小 = `2^order` 页。order 0 为单页 |
| **每个阶一条空闲链表** | `static free_area_t free_area[MAX_ORDER];`，`free_area[k]` 存所有 2^k 页的**空闲块头页** |
| **头页标记** | 空闲块的头页置 `PG_property`，`property = order` |
| **已分配块也记 order** | 分配后清 `PG_property`，但 `property` 仍保留该块的 order，供 `free_pages` 还原块大小 |
| **`nr_free` 是页数** | 维护一个全局空闲页计数，`alloc` 时减去 `2^order`，`free` 时加上 `2^order` |
| **`ref` 保持 0** | 与 `default_pmm.c` 的约定一致（其 `basic_check` 断言分配后 `page_ref == 0`） |

### 2.3 阶的上界

`MAX_ORDER = 16`（order 0~15）。理由：可管理物理内存 128 MiB ÷ 4 KiB = **32768 页 = 2^15**，所以最大块是 order 15。

```c
#define MAX_ORDER 16        /* 合法 order 为 0 .. MAX_ORDER-1 */
```

### 2.4 新增文件

| 文件 | 内容 |
|---|---|
| `kern/mm/buddy_pmm.c` | 实现六个成员函数 + 自研测试 |
| `kern/mm/buddy_pmm.h` | `extern const struct pmm_manager buddy_pmm_manager;` |

> **不需要改 Makefile**：`KSRCDIR` 已包含 `kern/mm`，构建系统会自动收集该目录下的 `.c`；`KINCLUDE` 也包含 `kern/mm`，头文件可直接 `#include <buddy_pmm.h>`。

---

## 三、核心算法

### 3.1 伙伴地址计算（最关键）

一个 order 为 `k`、起始于 `pages` 数组下标 `i` 的块，其**伙伴块**的下标是：

```c
buddy_index = i ^ (1u << k);
```

成立前提：`i` 必须是 `2^k` 的倍数（块必须对齐到自身大小）。

> 用 `pages` 数组下标（即 `page - pages`）而非全局物理页号来算。因为 Buddy 的"对半"是相对于**本管理器所管辖的区域起点**的，用它最直观；又因 `nbase = 0x80000 = 2^19` 是 2 的幂，对 order ≤ 15 而言两种算法结果一致。

### 3.2 初始化：`init` 与 `init_memmap`

**`init()`**：把 `MAX_ORDER` 条链表全部 `list_init`，`nr_free = 0`。

**`init_memmap(base, n)`** 有一个容易漏掉的点：`page_init()` 交来的区域**起点不一定按 2 的幂对齐、长度也不一定是 2 的幂**，不能直接当成一个大块登记，必须**分解成若干"对齐的 2 的幂块"**：

```
i0 = base - pages;              // 区域起点在 pages 中的下标
while (n > 0) {
    k = 满足以下条件的最大阶：
          (1<<k) <= n            // 不超过剩余页数
          (i0 & ((1<<k)-1)) == 0  // 起点对齐到该块大小
          k < MAX_ORDER
    add_block(base, k);          // 头页置 PG_property、property = k，插入 free_area[k]
    base += (1<<k);  i0 += (1<<k);  n -= (1<<k);
}
```

> 这一步做对了，后续的分配/合并才可能正确——**如果块没有按大小对齐，伙伴地址计算就是错的**。

**每页的初始化**：参照 `default_init_memmap`，逐页 `assert(PageReserved(p))`，然后清 `flags`、`ref` 置 0；只有**块头页**置 `PG_property` 并写 `property = k`。

### 3.3 分配：`alloc_pages(n)`

```
1. n == 0 → 返回 NULL（或用 assert 拦住）
2. 计算所需阶：k = ceil(log2(n))；若 k >= MAX_ORDER 或大于当前最大空闲块 → NULL
3. 从 k 开始向上找第一个非空的 free_area[m]（m >= k）
   - 找不到 → 返回 NULL（内存不足）
4. 分裂：从 m 一路降到 k+1：
   取出当前 order 的块，拆成两个 order-1 的块；
   一个插入 free_area[order-1]，另一个继续参与下一轮分裂
5. 最终剩下的那个 order=k 的块就是分配结果：
   从链表摘除、清 PG_property、property = k、nr_free -= (1<<k)
6. 返回该块的头页指针
```

> **注意**：`alloc_pages(n)` 返回的块大小是 `2^k ≥ n`，即"**≥ n 页**"，符合框架契约。

### 3.4 释放：`free_pages(base, n)`

```
1. 从 base->property 取出该块的 order k（分配时记下的）
   —— 注意不要用 n 反推，因为调用方传入的 n 可能小于实际块大小
2. nr_free += (1<<k)；头页置 PG_property、property = k
3. 尝试合并（循环）：
   while (k < MAX_ORDER-1) {
       buddy = pages + ((base - pages) ^ (1<<k));
       若 buddy 是空闲块头页（PageProperty(buddy)）且 buddy->property == k
           且 buddy 在该阶链表中：
           从 free_area[k] 摘下 buddy；
           合并后的头页取 base 与 buddy 中地址较小者；
           令 base = 合并头页，k = k + 1，更新其 property = k；
       else break;
   }
4. 把最终的块插入 free_area[k]
```

> **合并要递归向上**：两块合并成 order+1 后，新块可能还能与它的伙伴继续合并，直到不能再合。

### 3.5 `nr_free_pages()` 与 `check()`

- `nr_free_pages()` 直接返回维护的 `nr_free`。
- `check()` 调用自研测试（见第五节）。`pmm_init()` 会通过 `check_alloc_page()` 调它。

---

## 四、⚠️ 与自动评分的冲突（必须先想清楚）

`make grade` 的 `[Test 2]` 检查的是：

```c
if (pmm_manager != &best_fit_pmm_manager) → FAIL
```

也就是说，**最终提交的版本运行时必须是 `best_fit_pmm_manager`**。如果收口时把 `init_pmm_manager()` 切到 Buddy，`[Test 2]` 会直接 FAIL，20 分变 10 分。

**处理方案（推荐 A）：**

| 方案 | 做法 | 优点 | 缺点 |
|---|---|---|---|
| **A. 临时切换验证** | 开发/验证阶段临时把 `init_pmm_manager()` 切到 `buddy_pmm_manager`，看到自研测试全过、截图存证后，**改回 `best_fit_pmm_manager`** 再提交 | 最终 20/20 稳拿 | 提交版本里 Buddy 是未被调用的代码 |
| B. 编译开关 | `#ifdef USE_BUDDY` 决定用哪个管理器，默认 best_fit；报告说明如何打开 | 评审者能复现 | 多一处宏，略增复杂度 |

**决定：采用 A，并额外用一段注释说明 Buddy 的验证方式。** 报告里贴 Buddy 运行时的输出截图作为证据。

---

## 五、测试设计

自研测试放在 `buddy_pmm.c` 里（如 `buddy_check()`），全部用 `assert`，并在关键步骤 `cprintf` 打点。

### 5.1 必须覆盖的核心行为

| # | 测试点 | 预期 |
|---|---|---|
| 1 | **初始化后状态** | `nr_free_pages()` 等于 `page_init` 交给管理器前的空闲页总数；每条链表要么空、要么只含对齐块 |
| 2 | **分配单页** | `alloc_page()` 非空，`nr_free` 恰好减少 1 |
| 3 | **分配 2 的幂**（2/4/8 页） | 成功，`nr_free` 各减少对应页数 |
| 4 | **分配非 2 的幂**（如 3 页） | 成功，内部向上取整到 4 页，`nr_free` 减少 **4**（不是 3） |
| 5 | **分裂正确** | 只有大块时空闲时申请小块 → 能从大块中分裂出小块，且剩余块仍在链表中 |
| 6 | **释放单个不合并** | 只释放一块（伙伴仍被占用）→ `nr_free` 增加对应页数，**不**发生合并 |
| 7 | **伙伴合并** | 释放一对伙伴 → 合并成一块，高阶层链表多一个块 |
| 8 | **递归向上合并** | 连续分配 8 页再全部释放 → 应一路合并回 order 3 的大块 |
| 9 | **耗尽** | 一直分配直到返回 `NULL`，且 `nr_free == 0` |
| 10 | **不泄漏** | 反复"分配 N 页 → 释放 N 页"若干轮，`nr_free` 始终回到初始值 |

### 5.2 边界情况

| # | 场景 | 预期 |
|---|---|---|
| 11 | `alloc_pages(0)` | 返回 `NULL`（或 assert，取其一并在报告中说明） |
| 12 | 请求超过当前最大空闲块 | 返回 `NULL` |
| 13 | 请求超过 `2^(MAX_ORDER-1)` 页 | 返回 `NULL` |
| 14 | 释放后立即再分配同大小 | 能拿到刚释放的块 |
| 15 | 分配两个块、先释放低地址再释放高地址 / 反过来 | 两种顺序都应正确合并 |

### 5.3 为什么这些测试有效（报告要写）

- **伙伴地址算错** → 合并失败 → `nr_free` 比预期小、高阶层链表为空 → 测试 7/8 能直接抓出来；
- **`init_memmap` 没做对齐分解** → 块未对齐 → 后续分裂/合并的伙伴计算全错 → 测试 1/5 就能暴露；
- **合并没递归** → 测试 8 的 `nr_free` 虽然对，但高阶层块数量不对 → 测试 8 抓；
- **没有维护 `nr_free`** → 测试 1/2/9/10 全挂；
- **非 2 的幂取整错误** → 测试 4 的 `nr_free` 减量不对。

---

## 六、两份 prompt 的设计思路

### 6.1 第一份：生成 Buddy 模块

按 lab0.5 四段式组织：

| 段 | 要点 |
|---|---|
| `[PROMPT]` | 任务：在 `kern/mm/` 下新建 `buddy_pmm.c` / `buddy_pmm.h`，实现一个完整的 Buddy System 物理内存管理器并注册为 `buddy_pmm_manager`；**必须修改真实文件**；保留现有代码 |
| `[RELY]` | 给出真实上下文：`struct Page` 定义、`PG_property`/`SetPageProperty` 等宏、`page2ppn`/`page2pa`/`pa2page`、`struct pmm_manager` 六个成员的签名与语义、`free_area_t`、`list.h` 的链表 API、`assert`/`panic`/`cprintf`、`MAX_ORDER=16` 的依据（32768 页） |
| `[GUARANTEE]` | 必须交付：`buddy_init` / `buddy_init_memmap` / `buddy_alloc_pages` / `buddy_free_pages` / `buddy_nr_free_pages` / `buddy_check` + `buddy_pmm_manager` 结构 + 头文件 |
| `[SPECIFICATION]` | 按第三节的算法逐函数写清 **Pre-Condition / Post-Condition / Case / Requirements**，**重点强调三件事**：① 块必须按大小对齐；② `init_memmap` 要做对齐分解；③ 释放时必须**递归**向上合并 |

> **写 prompt 的关键**：把"对齐"和"递归合并"这两条写成 **Requirements 里的硬约束**，而不是笼统地说"正确实现 buddy 算法"——否则 AI 极易写出"伙伴地址用加减算"或"只合并一层"的实现。

### 6.2 第二份：设计测试

| 段 | 要点 |
|---|---|
| `[PROMPT]` | 任务：针对上一步生成的 Buddy 实现，设计测试函数并接入 `check()`；**必须修改真实文件** |
| `[RELY]` | 第一份生成的实现（函数名、内部状态）、`nr_free_pages()` 等可观测接口 |
| `[GUARANTEE]` | 必须交付：一个测试函数（覆盖 5.1 的 10 个核心行为 + 5.2 的 5 个边界） |
| `[SPECIFICATION]` | 每个测试点写清"输入 → 期望的 `nr_free`/返回值/链表状态"，并要求：测试要能**独立判断成败**（用 `assert`），并**还原状态**（跑完不破坏后续使用） |

> **测试 prompt 的难点**：AI 容易只写"能分配就行"，漏掉"合并"和"不泄漏"。所以 `[SPECIFICATION]` 里要**逐条列出必须覆盖的场景**（把第五节那张表搬进去）。

---

## 七、实现步骤

| 步 | 内容 | 产出 |
|---|---|---|
| 1 | 定下数据结构与约定（本文件二、三节） | 设计确认 |
| 2 | 写第一份 prompt → 让 AI 生成 `buddy_pmm.c/.h` | 可编译的模块 |
| 3 | 接入：临时把 `init_pmm_manager()` 切到 `buddy_pmm_manager` | `make qemu` 能跑 |
| 4 | 写第二份 prompt → 让 AI 生成测试 | `buddy_check()` |
| 5 | 跑测试、根据失败点迭代 prompt | 全部通过 |
| 6 | 截图存证（`make qemu` 输出） | `report/images/lab2_buddy.png` |
| 7 | **把 `init_pmm_manager()` 改回 `best_fit_pmm_manager`** | 满足 `[Test 2]` |
| 8 | 写 `prompt.md` 第三节 + 报告「任务三」 | 交付 |

---

## 八、易错点与风险

| # | 风险 | 说明 |
|---|---|---|
| 1 | **伙伴地址用加减算** | 必须是 `i ^ (1<<k)`，且块要对齐 |
| 2 | **`init_memmap` 没做对齐分解** | 直接把整段当一个块 → 块不对齐 → 后续全错 |
| 3 | **合并只做一层** | 必须 `while` 循环向上递归 |
| 4 | **用 `n` 反推已分配块的 order** | 调用方可能传入小于实际块大小的 `n`；应从 `base->property` 取 |
| 5 | **`nr_free` 单位搞错** | 是**页数**不是块数 |
| 6 | **忘了还原管理器** | 提交前必须切回 `best_fit_pmm_manager`，否则 `[Test 2]` FAIL |
| 7 | **新文件没被编进去** | 正常会自动收集 `kern/mm/*.c`，但若放在别的目录就要改 Makefile |
| 8 | **测试破坏状态** | 测试跑完要恢复，否则后续 `make qemu` 可能异常 |

---

## 九、验收标准

- [ ] `buddy_pmm.c` / `buddy_pmm.h` 编译通过（`-Werror` 下无警告）
- [ ] 临时切换后 `make qemu` 能跑到 `check_alloc_page() succeeded!`
- [ ] `buddy_check()` 覆盖第五节的 10 个核心行为 + 5 个边界，全部通过
- [ ] 截图 `report/images/lab2_buddy.png` 能看到测试通过输出
- [ ] **`init_pmm_manager()` 已改回 `best_fit_pmm_manager`**
- [ ] `make grade` 仍为 **20/20**
- [ ] 两份 prompt 已按四段式写入 `report/prompt.md` 第三节
- [ ] 报告「任务三」写了：两份 prompt、迭代过程、测试覆盖说明
