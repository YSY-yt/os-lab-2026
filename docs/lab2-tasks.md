# Lab 2 任务分工说明

> 本文件是本小组的内部工作文档，用于明确 lab2 三个部分的负责人、具体工作内容与交付要求。
> 仓库：<https://github.com/YSY-yt/os-lab-2026>　分支：`lab2`
> 实验指导书：<http://oslab.mobisys.top/lab2026/_book/lab2/lab2.html>

---

## 零、先读这一节

### 0.1 分工总览

| | 部分一 | 部分二 | 部分三 |
|---|---|---|---|
| **主题** | 物理内存初始化 | 页面分配算法 | 伙伴系统与收口 |
| **覆盖的练习** | 功能模块一 任务一 + Challenge 思考题 | 功能模块二 任务一 + 任务二 | 功能模块二 任务三 |
| **要写的代码** | `kern/mm/pmm.c` 的 `page_init()` | `kern/mm/best_fit_pmm.c` 整个模块 + `pmm.c` 切换管理器 | 新增 Buddy System 模块 + 自研测试 |
| **要交的 prompt** | 1 份 | 1 份 | **2 份** |
| **对应评分** | `[Test 1] Page Init: PASS`（10 分） | `[Test 2] Best-Fit: PASS`（10 分） | 无自动评分，靠自研测试与报告 |
| **难度** | 中 | 中 | **大** |
| **负责人** | **待定** | **待定** | **待定** |

**依赖关系**：部分一与部分二可**基本并行**（但两者的代码最终要一起编译）；部分三是独立模块，可与前两者并行开发，但**报告收口必须在三者都完成后**。

> ⚠️ **部分三明显比其他两块重**：Buddy System **实验不提供任何代码框架，也不提供测试函数**，要从零设计模块 + 自己设计验证方案 + 交两份 prompt。建议由动手能力最强的人承担。

**本次实验的决定：选做题只做「部分一」里的 Challenge 思考题（成本低、纯加分）；SLUB 那道选做题不做。**

### 0.2 提交规范（老师要求，不可违反）

- 一次实验一个分支：lab2 的成果**全部提交到 `lab2` 分支**，不要提交到 `main`。
- `lab2` 分支根目录下必须是两个文件夹：`code/`（实验代码）与 `report/`（报告）。
- `report/` 下必须包含：`report.md`、`prompt.md`、`images/`。
- 报告中的图片一律用**相对路径**引用，例如 `./images/lab2_grade.png`。
- **代码里 `//lab2 ... YOUR CODE` 的注释必须保留，并把 `YOUR CODE` 替换为自己的学号。**

### 0.3 环境：已经替你踩完坑了

**你不需要重复做环境适配**——仓库里的代码**已经改好了**，直接 `make` 就能用：

| 已做的改动 | 原因 |
|---|---|
| `Makefile` 的 `qemu`/`debug`/`test` 三个目标：`-device loader,...` → **`-kernel $(UCOREIMG)`** | QEMU ≥ 8 自带的 OpenSBI 是 `fw_dynamic` 变体，跳转地址不再硬编码、必须由 QEMU 经 `-kernel` 告知。用原写法 OpenSBI 的 `Domain0 Next Address` 是 `0x0`，**内核永远不会被执行** |
| `tools/grade.sh` 的 `qemuopts` 同样改 `-kernel` | 同上。**不改的话 `make grade` 恒为 0 分** |
| `Makefile` 的 `gdb` 目标改为**自动探测调试器** | 原写法硬编码 `riscv64-unknown-elf-gdb`，而 apt 仓库没有这个包（只有 `gdb-multiarch`） |

**三条避坑提醒：**

1. **必须在交互式 Ubuntu 终端里跑 `make`**（非交互 shell 加载不到工具链 `PATH`）。
2. **退出 QEMU**：按住 `Ctrl` 再按 `a`，两键都松开，再单独按一下 `x`。
3. **lab2 的代码交付时是编译不过的**——这是**预期的**。`best_fit_pmm.c` 报 `free_list` / `nr_free` 未声明，正是因为那两个宏本来就要按 `default_pmm.c` 的样子自己写。**看到这个报错不要慌，那是任务本身。**

**常用命令：**

```bash
cd ~/os-lab-2026/code     # 自备电脑；组长机器上是 /mnt/d/os-lab-2026/code
make                      # 编译
make qemu                 # 编译并运行
make grade                # 自动评分（lab2 起提供）
make debug / make gdb     # 调试（需两个终端）
pkill -f qemu-system-riscv64   # 清理卡住的 QEMU
```

### 0.4 自动评分是怎么打的

`make grade` 的流程：

```
make clean → 把整棵树复制到 /tmp/lab2_grade
          → python3 tools/inject.py <副本>     ← 把测试注入副本（防作弊，学生改不了）
          → cd 副本 && sh tools/grade.sh       ← 运行 QEMU，在串口输出里匹配结果
```

`grade.sh` 只检查两行，**各 10 分，共 20 分**：

```
[Test 1] Page Init: PASS      ← page_init() 的 8 条验收（见部分一）
[Test 2] Best-Fit: PASS       ← 仅仅检查 pmm_manager == &best_fit_pmm_manager
```

> **重要**：部分三（Buddy System）**没有自动评分**，因为实验不提供它的测试函数。它的验证完全靠你们自己设计的测试。

---

## 一、部分一：物理内存初始化

**负责人：待定**

### 1.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| `page_init()` 实现 | `code/kern/mm/pmm.c`（标有 `//lab2 function1 YOUR CODE` 处） | 通过 8 条验收；`YOUR CODE` 改成你的学号 |
| Challenge 思考题解答 | `report/report.md` 第四节「拓展任务 Challenge」 | 见 1.4 |
| 报告「功能模块一 任务一」 | `report/report.md` | 含最终提示词、实现迭代过程、遇到的问题与解决 |
| 1 份 prompt | `report/prompt.md` 第一节 | 四段式框架，写实 |

### 1.2 具体做什么

实现 `kern/mm/pmm.c` 里的 `static void page_init(void)`。它在 `pmm_init()` 中被调用，顺序是：

```
pmm_init() → init_pmm_manager() → page_init() → check_alloc_page()
```

**要实现的核心逻辑**（按依赖顺序）：

1. **设置地址映射偏移**：`va_pa_offset = PHYSICAL_MEMORY_OFFSET`（`0xFFFFFFFF40000000`）
2. **从设备树取物理内存范围**：`get_memory_base()` / `get_memory_size()`（这两个函数在 `kern/driver/dtb.c` 里已实现，直接用）。信息无效（返回 0）时要 `panic` 终止
3. **算可管理上界与页数**：`npage = PPN(mem_base + mem_size)`，且要保证 `npage > nbase`（`nbase = DRAM_BASE / PGSIZE`）
4. **放置 Page 描述符数组**：`pages` 要放在内核 `end` 之后、按 `PGSIZE` 对齐
5. **初始化描述符并标记保留区**：**内核镜像占用的页**和**描述符数组自身占用的页**都必须标成 `PageReserved`，绝不能进入空闲区
6. **把空闲页交给管理器**：对每个空闲连续区调用 `init_memmap(base, n)`；**没有任何完整页时不得调用**

**关键宏与工具（都在 `kern/mm/` 下）：**

| 名称 | 位置 | 用途 |
|---|---|---|
| `KERNBASE` / `PHYSICAL_MEMORY_OFFSET` / `PHYSICAL_MEMORY_END` | `memlayout.h` | 地址常量 |
| `KERNEL_BEGIN_PADDR` / `KERNEL_BEGIN_VADDR` | `memlayout.h` | 内核的物理/虚拟起始 |
| `struct Page`、`SetPageReserved` / `PageReserved` | `memlayout.h` | 页描述符与状态位 |
| `PPN()` / `PGSIZE` / `PGSHIFT` / `ROUNDUP` | `mmu.h` | 页号与对齐 |
| `page2pa()` / `pa2page()` / `page2ppn()` | `pmm.h` | 地址与描述符互转 |
| `PADDR()` / `KADDR()` | `pmm.h` | 虚拟地址 ↔ 物理地址 |
| `init_memmap()` / `nr_free_pages()` | `pmm.c` | 交给管理器 / 查空闲页数 |
| `end`（链接符号） | —— | 内核镜像结束位置 |

### 1.3 验收标准（8 条，全在 `tools/test_lab2.c` 里）

1. `va_pa_offset == PHYSICAL_MEMORY_OFFSET`
2. 设备树报告了有效内存范围（base、size 均非 0）
3. `npage == PPN(mem_base + mem_size)` 且 `npage > nbase`
4. `pages` 非空、按 `PGSIZE` 对齐、位于**内核 `end` 向上取整到页**之后
5. 空闲页数在 `(0, npage - nbase]` 区间内
6. **内核镜像尾页**（`PADDR(end)` 所在页）必须是 `PageReserved`
7. **页描述符数组自身所在页**（`PADDR(pages)` 所在页）必须是 `PageReserved`
8. `page2pa()` / `pa2page()` 往返转换一致

**自查命令**（实现后跑，必须看到 20/20 或至少 Test 1 通过）：

```bash
cd ~/os-lab-2026/code
make grade
```

### 1.4 Challenge 思考题（本次要做）

> **如果 OS 无法提前知道当前硬件的可用物理内存范围，请问你有何办法让 OS 获取可用物理内存范围？**

**这是本次唯一要做的选做题**，不用写代码，答得有理有据即可。可参考的方向（不限于此）：

- **固件/引导程序传递**：OpenSBI 通过 `a1` 把设备树（DTB）地址交给内核——**本实验走的就是这条路**
- **固件提供的查询接口**：SBI 的内存相关调用
- **平台固件表**：x86 上的 ACPI E820 内存映射表 / UEFI 内存映射
- **硬件自描述机制**：设备树、PCI 枚举、SPD/EEPROM
- **主动探测法**：逐段读写测试（风险是可能踩到设备寄存器）

**写的时候要点**：说明每个方案的**适用场景与局限**，并明确指出**本实验用的是哪一种**。

### 1.5 完成标准（自检）

- [ ] `make grade` 中 `[Test 1] Page Init: PASS`
- [ ] 8 条验收标准我都能说出对应的代码在哪
- [ ] `YOUR CODE` 已替换为我的学号，`//lab2` 注释保留
- [ ] `report/prompt.md` 第一节的提示词已按四段式填全
- [ ] 报告里写了「AI 生成的代码出了什么 bug、怎么解决的」
- [ ] Challenge 思考题已作答

### 1.6 易错点

- ❌ **忘记保留描述符数组自身**——这是最容易漏的一条（第 7 条验收专门查它）。
- ❌ 把**虚拟地址**当成物理地址算页号。记住：`pages` 是虚拟地址，算物理地址要用 `PADDR()`。
- ❌ `npage` 算成 `PPN(mem_size)`——应该是 `PPN(mem_base + mem_size)`。
- ❌ 忘记页对齐，或者对齐用错方向。
- ❌ 设备树信息无效时没有 `panic`，导致后面越界访问。

---

## 二、部分二：页面分配算法

**负责人：待定**

### 2.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| First-Fit 思考题解答 | `report/report.md` 第四节「功能模块二 任务一」 | 见 2.3 |
| Best-Fit 完整模块 | `code/kern/mm/best_fit_pmm.c`（`//lab2 function2-2 YOUR CODE` 处往下） | 通过两个测试函数 |
| 管理器切换 | `code/kern/mm/pmm.c` 的 `init_pmm_manager()` | 切成 `best_fit_pmm_manager` |
| Best-Fit 改进空间回答 | `report/report.md` | 见 2.6 |
| 1 份 prompt | `report/prompt.md` 第二节 | 四段式 |

### 2.2 具体做什么

**第一步：先读懂 First-Fit**（这也是任务一的要求）

仔细读 `code/kern/mm/default_pmm.c` 的四个函数：

| 函数 | 作用 |
|---|---|
| `default_init` | 初始化空闲链表与空闲页计数 |
| `default_init_memmap` | 把一段连续物理页登记为空闲块：首部页标记 `PG_property` 并记录块大小，插入链表 |
| `default_alloc_pages` | 从**链表头**开始找**第一个** `property >= n` 的块；够大就切分，剩余部分作为新块插回 |
| `default_free_pages` | 页标记回空闲、插回链表，并**向前/向后合并**相邻空闲块 |

**第二步：写 Best-Fit**

`best_fit_pmm.c` 交付时**只有两个测试函数**（`basic_check` / `best_fit_check`），**没有任何管理函数的声明和注册结构**。你需要**自己设计**：

```c
static free_area_t free_area;          // 内部状态，参照 default_pmm.c 的写法
#define free_list (free_area.free_list)
#define nr_free   (free_area.nr_free)

static void      best_fit_init(void);
static void      best_fit_init_memmap(struct Page *base, size_t n);
static struct Page *best_fit_alloc_pages(size_t n);
static void      best_fit_free_pages(struct Page *base, size_t n);
static size_t    best_fit_nr_free_pages(void);

const struct pmm_manager best_fit_pmm_manager = { ... };   // 注册
```

**Best-Fit 与 First-Fit 的唯一区别在分配**：不是遇到第一个够用的就分，而是**遍历整个空闲链表，挑出「满足需求且块最小」的那个块**。

**第三步：切换管理器（不做这一步等于白做）**

`pmm.c` 的 `init_pmm_manager()` 里现在是：
```c
pmm_manager = &default_pmm_manager;      // 改成 &best_fit_pmm_manager
```
`[Test 2]` 就是专门查这一条的——只改了 `best_fit_pmm.c` 而不切换，`[Test 2]` 一定 FAIL。

**禁止改动**：`basic_check` 与 `best_fit_check` 两个测试函数**一个字都不能动**。

### 2.3 First-Fit 思考题

> 结合上述四个函数，用自己的话简要说明 First-Fit 分配器如何**初始化空闲页**、**分配连续物理页**、**回收页面并合并相邻空闲块**。同时回答：**原有的 first-fit 算法是否有进一步的改进空间？**

**改进空间的参考方向**：分配是 **O(n) 线性扫描**；块大小没有做索引（可按大小维护多级链表 / 分离适配）；切分策略可能留下大量小碎片；插入链表时按地址排序保证能合并，但每次插入也是 O(n)。

### 2.4 验收标准

- `make grade` 输出 `[Test 2] Best-Fit: PASS`（10 分）
- `best_fit_check()` 内部 6 个计分点全部走到（`ucore_test` 宏打开时会打印 `grading: N / 6 points`）
- 编译无警告（`CFLAGS` 带 `-Werror`，警告即错误）

### 2.5 完成标准（自检）

- [ ] `make grade` 中 `[Test 2] Best-Fit: PASS`
- [ ] `init_pmm_manager()` 已切换为 `best_fit_pmm_manager`
- [ ] 两个测试函数**没有被改动**
- [ ] 运行 `make qemu` 时输出显示 `memory management: best_fit_pmm_manager`
- [ ] `YOUR CODE` 已替换为我的学号
- [ ] `report/prompt.md` 第二节已填全；报告里写了 bug 与解决过程

### 2.6 需要回答的问题

> **你的 Best-Fit 算法是否有进一步的改进空间？**

参考方向：Best-Fit 仍然是 **O(n) 线性扫描**（要遍历整条链表才能确定"最小够用"）；可维护**按块大小排序**的结构（如红黑树）把查找降到 O(log n)；可设置**最小块阈值**避免切出过小的碎片；可改为**分离适配**（segregated fit）。此外 Best-Fit 的特点决定了它容易产生大量极小碎片（外部碎片问题反而比 First-Fit 更明显）。

### 2.7 易错点

- ❌ **只写 `best_fit_pmm.c` 而忘了切换 `pmm.c` 的 `pmm_manager`**——最常见也最致命。
- ❌ 修改了 `basic_check` / `best_fit_check`，导致注入后的测试与你的预期不符。
- ❌ 释放时**合并相邻空闲块**的条件写错（应该比较"前一块的起点 + 块大小 == 当前块起点"）。
- ❌ 分配后忘记 `nr_free -= n` 或忘记清除首部页的 `PG_property`。
- ❌ 切分时新块的头页忘记 `SetPageProperty`。

---

## 三、部分三：伙伴系统与收口

**负责人：待定**

### 3.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| Buddy System 模块 | `code/kern/mm/` 下**新建文件**（如 `buddy_pmm.c`/`.h`），并接入内核 | 能编译、`make qemu` 可运行 |
| 自研测试函数 | 同模块或单独文件 | 覆盖核心行为与边界情况 |
| **2 份 prompt** | `report/prompt.md` 第三节 | 第一份生成模块、第二份设计测试 |
| 报告「功能模块二 任务三」 | `report/report.md` | 含两份 prompt、迭代过程、测试覆盖说明 |
| 报告汇总与收口 | `report/report.md`、`report/prompt.md` | 见 3.6 |

### 3.2 具体做什么

**这是本次最重的一块：实验不提供 Buddy System 的代码框架，也不提供测试函数**，具体实现方式和测试方案由你们自己分析设计。

**算法要点：**

1. 把可用空间划分为大小为 **2 的 n 次幂**的块（1, 2, 4, 8, 16, 32, 64, 128 … 页）
2. **分配**：把请求大小向上取整到 2 的幂；若没有正好合适的块，就取更大的块**不断对半分裂**直到合适
3. **释放**：检查其**伙伴块**（与自己配成一对的另一半）是否也空闲，是则**合并**成更大的块，并**递归向上**继续尝试合并
4. **伙伴块的地址计算**是关键：同级块中，两块只有第 `k+1` 位不同（`k` 为块的阶数），可用异或算出：`buddy = base ^ (1 << order)`
5. 时空复杂度不做要求，**能通过测试即可**

**接入方式**：仿照 `default_pmm.c` / `best_fit_pmm.c` 实现一组 `xxx_*` 管理函数 + 一个 `const struct pmm_manager xxx_pmm_manager`，然后在 `pmm.c` 的 `init_pmm_manager()` 中切换过去。

**两份 prompt 的分工：**

| | 内容 |
|---|---|
| **第一份** | 让 AI **参考现有 ucore 物理内存管理框架**，从零生成 Buddy System 分配模块并接入内核 |
| **第二份** | 让 AI **针对第一份生成的算法设计测试函数**，检验分配与释放是否正确 |

### 3.3 测试要覆盖什么（报告里要说明）

**核心行为**：按 2 的幂分配、块分裂、释放、伙伴合并、**合并后递归向上**、连续分配耗尽后的行为。

**边界情况**：请求非 2 的幂大小、请求超过最大块、申请 1 页、每次分配后 `nr_free_pages()` 是否正确、**反复分配-释放后不应泄漏**、分配了 A 和 B 后释放 A 再释放 B 能否正确合并。

**为什么这些测试有效**：报告里要给出理由——例如"如果伙伴块地址算错，合并就会失败，`nr_free_pages()` 会比预期小，测试能直接抓出来"。

### 3.4 完成标准（自检）

- [ ] Buddy 模块能编译通过（`-Werror` 下无警告）
- [ ] `make qemu` 能跑到 `check_alloc_page() succeeded!`
- [ ] 自研测试全部通过，且在 `make qemu` 的输出里可见
- [ ] **两份 prompt** 都已按四段式填进 `report/prompt.md`
- [ ] 报告里回答了「测试覆盖了哪些核心行为和边界情况、为什么有效」
- [ ] 报告里写了 AI 生成过程中出现的 bug 与解决过程

### 3.5 易错点

- ❌ **伙伴块地址算错**——最常见。记住是"与自己在同一对半区域内的另一半"，用异或算，不是简单加减。
- ❌ 合并只做一层、忘了**递归向上合并**。
- ❌ 待分配空间不是 2 的幂时忘记向上取整。
- ❌ 释放时没有把自己标回空闲就急着合并。
- ❌ 自研测试只测了"能分配"，没测"释放后能合并、不泄漏"。

### 3.6 收口工作（部分三附带）

在三人各自完成后，部分三负责人负责汇总与收口：

- 填 `report/report.md` 的第三节（整体逻辑分析）、第二节（实验环境表）、第六节（总结与收获）
- 填报告头的「小组分工」表与「完成日期」
- 汇总 `report/prompt.md`，补齐迭代记录
- 走一遍下面的收口清单

---

## 四、收口清单（提交前逐条打勾）

**内容完整性**

- [ ] `report/report.md` 六节齐全，**全文没有 `[待填写]`、`[待定]`、`[YYYY-MM-DD]`、`[待本人确认]` 这类占位符**
- [ ] 报告七项要求全部覆盖，**特别是 lab2 新增的两项**：④「每个练习的 spec 是怎么写的、prompt 是怎么写的」⑤「AI 生成代码过程中出现了什么 bug、怎么解决的」
- [ ] 两道思考题（First-Fit 改进空间、Best-Fit 改进空间）都已作答
- [ ] Challenge 思考题（可用物理内存范围获取方法）已作答
- [ ] Buddy 的「测试覆盖说明」已作答
- [ ] 三处「**负责人：**」都已填上具体学号-姓名
- [ ] 「小组分工」表 3 行都已填实

**代码**

- [ ] `cd code && make` 在 `-Werror` 下无警告通过
- [ ] `make qemu` 能跑到 `check_alloc_page() succeeded!`，且显示 `memory management: best_fit_pmm_manager`
- [ ] `make grade` 得到 **20/20**
- [ ] 所有 `YOUR CODE` 已替换为学号，`//lab2` 注释保留
- [ ] `bin/`、`obj/` 等编译产物**没有**被提交

**图片**

- [ ] `report/images/` 下有 `make qemu` 与 `make grade` 两张截图
- [ ] `report.md` 里引用路径为 `./images/xxx.png`，文件名与磁盘**完全一致**
- [ ] 在 GitHub 网页上打开 `report.md`，两张图能正常显示

**分支与推送**

- [ ] 所有成果在 `lab2` 分支上，没有提交到 `main`
- [ ] `lab2` 分支根目录下正好是 `code/` 与 `report/` 两个文件夹
- [ ] 已 `git push` 成功，GitHub 网页上能看到最新提交
- [ ] 提交时间早于老师要求的截止时间

---

## 五、附录：命令速查

```bash
# 进入代码目录（自备电脑）
cd ~/os-lab-2026/code

make                    # 编译
make qemu               # 编译并运行（退出：Ctrl+A 松开再按 x）
make grade              # 自动评分（20 分）
make clean              # 清理

# 调试（两个终端）
make debug              # 终端 A
make gdb                # 终端 B

# 查看内核信息
riscv64-unknown-elf-objdump -d bin/kernel | less
riscv64-unknown-elf-nm bin/kernel | grep -E "pages|npage|free"

# 清理卡住的 QEMU
pkill -f qemu-system-riscv64

# 提交
cd ~/os-lab-2026
git pull
git status
git add -A
git commit -m "lab2: 说明你改了什么"
git push
```

---

## 六、参考：lab2 官方要求原文

**实验目的**（`lab2_1_goal.html`）：

> 理解页表的建立和使用方法
> 理解物理内存的管理方法
> 理解页面分配算法

**练习**（`lab2_2_1_exercise.html`）：

> 本实验围绕物理内存的发现、初始化和分配展开：内核首先选择并初始化物理内存管理器，再从设备树获取物理内存范围，由 `page_init()` 建立物理页描述符并将可用页面交给物理内存管理器；随后在理解 First-Fit 分配流程的基础上，实现并接入 Best-Fit 分配器，并通过 Buddy System 和 SLUB 扩展任务进一步理解页级分配与任意大小内存对象分配。整体实现链可以概括为 `pmm_init() -> init_pmm_manager() -> page_init() -> init_memmap() -> alloc_pages()/free_pages()`。

**报告要求**（`lab2_5_labs_requirement.html`）：

> 1. 基于 markdown 格式完成实验报告，以文本说明为主。
> 2. 说明自己对本章节整体逻辑的理解，即本章节是按照什么顺序、围绕哪些功能逐步实现的。
> 3. 说明自己对每个功能的核心函数或功能模块的理解。
> 4. 说明每个练习的 spec 是怎么写的，prompt 是怎么写的。
> 5. 说明在使用提示词让 AI 生成代码的过程中出现了什么 bug，以及你是怎么解决的。
> 6. 列出你认为本实验中重要的知识点，以及与对应的 OS 原理中的知识点，并简要说明二者的含义、关系和差异。
> 7. 列出你认为 OS 原理中很重要、但在本实验中没有对应上的知识点。
>
> 注意有"lab2"的注释，代码中所有需要完成的地方（challenge 除外）都有"lab2"和"YOUR CODE"的注释，请在提交时特别注意保持注释，并将"YOUR CODE"替换为自己的学号，并把所有标有对应注释的部分填上正确的代码。

**加分规定**（练习页末尾）：

> Challenges 是选做，完成 Challenge 并回答了助教问题的小组可获得本次实验的加分。

> 注：文档中**未给出具体的加分分值**，需向助教确认。
