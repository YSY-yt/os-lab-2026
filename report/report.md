# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 2: 物理内存和页表 |
| **小组成员** | 2412796-杨斯源（组长）、2413628-黄乐仁、2410868-孙锘梵 |
| **完成日期** | [YYYY-MM-DD] |

### 小组分工

练习如何分工？

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2412796-杨斯源（组长） | 部分三：伙伴系统与收口（Buddy System 模块 + 自研测试 + 报告汇总） |
| 2413628-黄乐仁 | 部分一：物理内存初始化（`page_init()` + Challenge 思考题） |
| 2410868-孙锘梵 | 部分二：页面分配算法（First-Fit 思考题 + Best-Fit 实现） |

实验报告如何分工？

报告的撰写按上表划分：各功能模块由其负责人撰写对应章节；实验环境、总结与收获及 `prompt.md` 的汇总由部分三负责人负责；最后经统一校对格式与图片引用后提交。

---

## 一、实验目的

本实验的主要目的是：

1. **理解物理内存的管理方法**：掌握操作系统如何发现可用物理内存、如何用页描述符组织物理页、以及如何把空闲页交给内存管理器。
2. **理解页面分配算法**：通过阅读 First-Fit 实现掌握连续物理内存分配的基本流程，并自己实现 Best-Fit 与 Buddy System 两种分配算法。
3. **理解页表的建立和使用方法**：理解 RISC-V Sv39 分页机制，掌握内核从物理地址 `0x80200000` 被搬到高虚拟地址 `0xFFFFFFFFC0200000` 后，虚拟地址与物理地址如何通过页表相互转换。
4. **学习用 AI 辅助开发操作系统核心功能**：按 lab0.5 的四段式框架设计提示词，让 AI 生成物理内存管理模块，并对生成结果做验证与迭代。

---

## 二、实验环境

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| 2412796-杨斯源 | [待本人确认] | [待本人确认] | |
| 2413628-黄乐仁 | Claude | DeepSeek V4 Flash | |
| 2410868-孙锘梵 | Cline（VS Code 插件） | DeepSeek V4 Pro | |

**运行环境：**

| 组件 | 版本 / 包名 |
|------|-------------|
| 操作系统 | Ubuntu 24.04.5 LTS（WSL2） |
| 交叉编译器 | `riscv64-unknown-elf-gcc` 15.1.0 |
| 二进制工具 | `riscv64-unknown-elf-ld` / `objcopy` / `objdump` |
| 模拟器 | `qemu-system-riscv64` 8.2.2（apt 包 `qemu-system-misc`，自带 OpenSBI v1.3） |
| 调试器 | `riscv64-unknown-elf-gdb` 16.3.90 |

**说明：**

- **AI 编程工具**：指具体使用的终端工具、编辑器插件、桌面应用或浏览器界面
- **底层模型**：指该工具使用的大语言模型及版本

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

<!-- 由部分三负责人起稿，全员过目 -->
<!-- 建议围绕：本章回答"操作系统怎样管理物理内存、怎样建立虚拟到物理的映射"。
     交付物是：内核能被搬到高虚拟地址运行（分页已开启），并能识别、组织、分配可用物理内存。
     核心链路：pmm_init() → init_pmm_manager() → page_init() → init_memmap() → alloc_pages()/free_pages() -->

[待填写]

### 3.2 功能的逐步实现

<!-- 按"为什么是这个顺序"来讲，建议顺序：
     1. 链接脚本把内核基址改到高虚拟地址 → 为什么必须先做（地址相关的代码在链接时就定了）
     2. entry.S 建立启动页表并开启 Sv39 → 为什么（内核要在分页模式下运行）
     3. 从设备树探测物理内存范围（dtb_init）→ 为什么（OS 事先不知道内存布局）
     4. page_init() 建立 Page 描述符数组、排除内核与描述符自身占用 → 为什么（要给每个物理页一个"身份证"）
     5. init_memmap() 把空闲页交给 pmm_manager → 为什么（管理器的输入）
     6. 实现并切换分配算法（First-Fit → Best-Fit → Buddy）→ 为什么（不同算法解决不同碎片问题） -->

[待填写]

---

## 四、实验内容与实现

<!--
说明：按练习文档中的功能模块与任务顺序组织。
每个任务下需写清：涉及函数、功能说明、最终提示词、实现迭代过程与遇到的问题。
课程明确要求报告中说明「每个练习的 spec 是怎么写的、prompt 是怎么写的」，
以及「使用提示词让 AI 生成代码的过程中出现了什么 bug，以及你是怎么解决的」。
-->

### 功能模块一：物理内存初始化

**负责人：** 2413628-黄乐仁

#### 任务一：实现物理内存探测与页描述符初始化功能

**需要实现/修改的函数：**

```c
static void page_init(void);   // kern/mm/pmm.c
```

**功能说明：**

根据设备树提供的物理内存信息建立可管理物理页的描述符布局，并把排除 OpenSBI、内核镜像与描述符数组占用空间后的完整空闲页面交给当前 `pmm_manager` 管理，为后续页面分配提供正确的初始状态。`page_init()` 不接收参数也不返回结果，由 `pmm_init()` 调用。

**实现约束：**

- 必须区分**虚拟地址、物理地址、物理页号、Page 描述符地址**四者
- 正确处理可管理上界与页面对齐
- 确保内核镜像与描述符数组**不会进入空闲区**
- 设备树信息无效时应终止初始化；**没有可用完整页面时不得调用 `init_memmap()`**

**验收标准（来自 `tools/test_lab2.c`，共 8 条）：**

1. `va_pa_offset == PHYSICAL_MEMORY_OFFSET`
2. 设备树报告了有效的物理内存范围（`get_memory_base()` / `get_memory_size()` 均非 0）
3. `npage == PPN(mem_base + mem_size)` 且 `npage > nbase`
4. `pages` 非空、按 `PGSIZE` 对齐、且位于内核 `end`（向上取整到页）之后
5. 空闲页数在 `(0, npage - nbase]` 区间内
6. 内核镜像尾页（`PADDR(end)` 对应的页）必须处于 `PageReserved` 状态
7. 页描述符数组自身所占的页也必须处于 `PageReserved` 状态
8. `page2pa()` / `pa2page()` 往返转换正确

**最终提示词：**

<!-- 见 report/prompt.md 的对应小节，此处粘贴最终定稿版本 -->

**实现迭代过程：**

**第一轮：生成初版。** 让 AI 参考经典 ucore 的 `page_init()` 生成代码。初版照搬 x86 版思路，用 `e820map` 结构探测物理内存，但本实验是 RISC-V 移植、没有 `e820map`：内存范围由 `dtb_init()` 解析设备树后经 `get_memory_base()/get_memory_size()` 提供，虚实映射偏移是 `PHYSICAL_MEMORY_OFFSET`（`0xFFFFFFFF40000000`）。因此第一版无法直接使用。

**第二轮：按 RISC-V 平台重写，并逐条对照 8 条验收。** 修正提示词，明确用设备树接口与平台常量，实现顺序为：设 `va_pa_offset` → 取 `mem_base/mem_size`（无效则 `panic`）→ 算 `npage = PPN(mem_end)` → 把 `pages` 放到内核 `end` 之后按页对齐 → 全量 `SetPageReserved` → 从 `freemem`（pages 数组末尾，需 `PADDR`）之后 `init_memmap` 释放空闲页。过程中重点排除了两处 bug：

1. **地址类型混淆**：`end` 是内核**虚拟地址**，直接当物理地址用会算错；必须先 `ROUNDUP((uintptr_t)end, PGSIZE)` 再 `PADDR` 转物理地址。`freemem` 同样是"pages 数组末尾"的虚拟地址，`PADDR` 之后才能与物理的 `mem_end` 比较。
2. **遗漏保留区**：不能只保留内核镜像占用的页，**页描述符数组自身占用的页也必须 `PageReserved`**，否则数组所在页会被当作空闲页交给管理器。实现里用"先全部 `SetPageReserved`、再从 `freemem` 之后释放"一次性覆盖这两块保留区，正好同时满足验收第 6、7 条。

**第三轮：编译与验收收敛。** 修正后在 `-Werror` 下编译通过、零警告，整棵树（含部分二的 `best_fit_pmm.c`、部分三的 `buddy_pmm.c`）成功链接；逐条核对 `tools/test_lab2.c` 的 8 条断言（`va_pa_offset`、`npage`、`pages` 对齐、空闲页数区间、内核尾页与数组页均 reserved、`page2pa`/`pa2page` 往返），最终 `make grade` 中 `[Test 1] Page Init: PASS`。

---

#### 拓展任务 Challenge：硬件的可用物理内存范围的获取方法（思考题）

**负责人：** 2413628-黄乐仁

**题目：** 如果 OS 无法提前知道当前硬件的可用物理内存范围，请问你有何办法让 OS 获取可用物理内存范围？

**解答：**

OS 在启动早期并不知道硬件内存布局，需要一套机制来"发现"可用内存。常见方案有以下几类：

**1. 依赖固件 / 引导程序传递（本实验采用的方式）**

RISC-V 平台下，OpenSBI 作为运行在 M 态的固件，在跳转到内核前会把设备树（DTB）的物理地址通过寄存器 `a1` 传给内核（`a0` 传 hartid）。内核解析 DTB 中 `memory` 节点的 `reg` 属性（起始地址 + 大小），即可得到可用物理内存范围。本实验正是这条路：`kern_init()` 先调 `dtb_init()` 解析设备树，`page_init()` 再读 `get_memory_base()/get_memory_size()` 得到 `0x80000000 + 128 MiB`。

- **适用场景**：嵌入式、RISC-V/ARM 等硬件差异大、没有统一 BIOS 的平台，由固件统一描述硬件。
- **局限**：依赖引导固件与内核之间的传递协议；DTB 内容由平台固件生成，若固件描述错误，内核无法自行纠偏。

**2. 固件提供的查询接口（SBI memory 调用）**

SBI 规范提供了内存相关的 ecall 接口（以及通过设备树/ACPI 等固件接口查询内存），内核可主动向 M 态固件发起查询。

- **适用场景**：有统一固件抽象层的平台。
- **局限**：能力受固件实现限制，不同 SBI 版本接口差异较大，可移植性一般。

**3. 平台固件表（ACPI E820 / UEFI 内存映射）**

x86 平台由 BIOS/UEFI 在启动时探测内存并填好 E820 内存映射表（或 UEFI 的 `GetMemoryMap()`），内核读取该表即可知道哪些区间可用、哪些被保留给设备或固件。

- **适用场景**：x86 PC/服务器，硬件标准化、有统一 BIOS/UEFI。
- **局限**：强依赖特定平台固件，不是跨平台通用方案。

**4. 硬件自描述机制（设备树、PCI 枚举、SPD/EEPROM）**

设备树本身就是硬件的自描述数据；此外 PCI 总线枚举能发现外设的 MMIO 区间，内存条上的 SPD/EEPROM 记录了 DIMM 容量，可用于估算内存大小。

- **适用场景**：设备树用于嵌入式/ARM/RISC-V；SPD 用于带标准 DIMM 的平台。
- **局限**：设备树仍需固件提供；SPD 只能得到容量，无法得到完整的"可用/保留"区间图。

**5. 主动探测法（逐段读写测试）**

内核依次对物理地址区间做写入-读回校验，能成功读回的即视为可用内存。

- **适用场景**：极端裸机环境、无任何固件描述。
- **局限**：风险很高——对 MMIO 设备寄存器区域读写可能触发副作用甚至死机，也无法区分"保留给固件"的内存，一般不推荐。

**结论**：本实验采用第 1 种方案——OpenSBI 通过 `a1` 把设备树 DTB 的物理地址传给内核，内核解析出 `memory` 节点的 base/size 后由 `page_init()` 建立页描述符。这是 RISC-V/ARM 平台最主流、最可靠的做法。

---

### 功能模块二：物理内存分配

**负责人：** 2410868-孙锘梵

#### 任务一：理解 first-fit 连续物理内存分配算法（思考题）

**负责人：** 2410868-孙锘梵

**题目：** 请结合 `kern/mm/default_pmm.c` 中的 `default_init`、`default_init_memmap`、`default_alloc_pages`、`default_free_pages` 四个函数，用自己的话简要说明 First-Fit 分配器如何初始化空闲页、分配连续物理页、回收页面并合并相邻空闲块。同时回答：**原有的 first-fit 算法是否有进一步的改进空间？**

**解答：**

First-Fit 分配器用 `free_list` 双向链表记录所有空闲块（每个空闲块由块首页代表，`property` 字段记块大小、`PG_property` 位标记块头），并用 `nr_free` 统计空闲页总数。链表始终按物理地址升序排列，这是能快速合并相邻块的前提。

**初始化空闲页**：`default_init` 把空闲链表初始化为空、`nr_free` 清零；`default_init_memmap` 把一段连续页登记为一个空闲块——先清空各页的标志位与引用计数，再在块首页设置 `property = n` 并打上 `PG_property` 标记，最后按地址升序把块插入链表。

**分配连续物理页**：`default_alloc_pages` 从链表头开始线性扫描，找到第一个大小满足 `property >= n` 的空闲块（"First-Fit"即第一个够用的就分）。若块比需求大则切分，剩余部分作为新块插回原位置，更新 `nr_free`、清除块头的 `PG_property` 标记后返回。

**回收并合并相邻空闲块**：`default_free_pages` 先把释放的页清零并重新标记为空闲块，按地址插回链表；再检查它的前驱和后继，若物理地址正好相邻就合并成一个更大的块，从而减少外部碎片。

**改进空间**：首先分配采用 O(n) 线性扫描、空闲块也没有按大小索引，查找效率低；其次切分策略容易在链表里积累大量小碎片。可考虑按块大小维护多级链表/分离适配，或用平衡树做大小索引来优化。

---

#### 任务二：实现 Best-Fit 连续物理内存分配算法

**负责人：** 2410868-孙锘梵

**需要实现/修改的函数与结构：**

```c
// kern/mm/best_fit_pmm.c —— 文件只提供 basic_check() 与 best_fit_check() 两个测试函数，
// 不提供任何管理函数的声明与注册结构，需自行设计
static void      best_fit_init(void);
static void      best_fit_init_memmap(struct Page *base, size_t n);
static struct Page *best_fit_alloc_pages(size_t n);
static void      best_fit_free_pages(struct Page *base, size_t n);
static size_t    best_fit_nr_free_pages(void);
static void      best_fit_check(void);

const struct pmm_manager best_fit_pmm_manager;   // 自行定义并注册

// kern/mm/pmm.c
static void init_pmm_manager(void);   // 需把 pmm_manager 切换为 best_fit_pmm_manager
```

**功能说明：**

参考 `default_pmm.c` 的完整 First-Fit 实现，设计 prompt 让 AI 在 `best_fit_pmm.c` 中标有 `lab2` 和 `YOUR CODE` 的位置实现完整的 Best-Fit 物理内存管理模块。

- Best-Fit 与 First-Fit 的区别：分配时**遍历整个空闲链表，选择满足需求且块大小最小的那个块**，而不是遇到第一个够用的就分配
- 生成的实现必须符合 `basic_check()` / `best_fit_check()` 使用的状态约定，**不得修改这两个测试函数**
- 完成模块后，**必须在 `kern/mm/pmm.c` 的 `init_pmm_manager()` 中把 `pmm_manager` 切换为 `best_fit_pmm_manager`**，否则内核运行的仍是 First-Fit

**验收标准：**

- `make grade` 中 `[Test 2] Best-Fit: PASS`（`pmm_manager == &best_fit_pmm_manager`）
- `best_fit_check()` 内部 6 个计分点全部通过

**最终提示词：**

````markdown
[PROMPT]
改kern/mm/best_fit_pmm.c,参照default_pmm.c补齐Best-Fit模块骨架让工程能编译。文件现在只有basic_check和best_fit_check两个测试函数,本阶段先按First-Fit实现。保留现有代码,只在//lab2标记处往下补。

[RELY]
free_area_t、Page结构体及页标志宏见kern/mm/memlayout.h,list_*见libs/list.h,set_page_ref和pmm_manager接口见kern/mm/pmm.h,best_fit_pmm.h已声明best_fit_pmm_manager。

[GUARANTEE]
实现best_fit_init/init_memmap/alloc_pages/free_pages/nr_free_pages,注册best_fit_pmm_manager(.check=best_fit_check),可加free_area和free_list/nr_free宏。行为同default_pmm.c对应函数。

[SPECIFICATION]

[PROMPT]
分析default_pmm.c(First-Fit)与Best-Fit的差异,给出best_fit_alloc_pages的修改规划,本次不改代码只输出规划。

[RELY]
best_fit_pmm.c的best_fit_alloc_pages现在是First-Fit:遇到第一个property>=n就返回。两算法共用同一数据结构(地址升序链表+块首页property记大小),区别只在选哪个块。

[GUARANTEE]
输出:唯一要改的函数、要改的代码段(查找循环)、改后选择逻辑、哪些函数不用改及原因。结论是只改best_fit_alloc_pages的查找循环,从取第一个够用块改为遍历整条链表取property最小的够用块,其余函数不改。

[SPECIFICATION]

[PROMPT]
根据刚才的分析改kern/mm/best_fit_pmm.c,把best_fit_alloc_pages从First-Fit改成Best-Fit。只改best_fit_alloc_pages,其他函数和两个测试函数不动。

[RELY]
当前best_fit_alloc_pages是First-Fit:循环里遇到第一个property>=n就break。Best-Fit要求遍历整条free_list,在property>=n的块里选property最小的。

[GUARANTEE]
只改best_fit_alloc_pages的查找循环,切分、nr_free更新、清PG_property等逻辑不变。

[SPECIFICATION]
遍历整条链表选property>=n且最小者,摘除、够大切剩余块插回、nr_free减n、清块头PG_property后返回;n>nr_free或无满足块返NULL。
````

**实现迭代过程：**

**第一轮：生成可编译的空壳。** `best_fit_pmm.c` 只有两个测试函数，且 `free_list`、`nr_free` 尚未声明，导致编译不过。于是先参照 `default_pmm.c` 补齐 `free_area` 与 `free_list` / `nr_free` 两个宏、五个管理函数（`best_fit_init`、`best_fit_init_memmap`、`best_fit_alloc_pages`、`best_fit_free_pages`、`best_fit_nr_free_pages`）以及末尾的 `best_fit_pmm_manager` 注册结构，此时分配逻辑仍是 First-Fit，但整棵树已能编译。

**第二轮：把分配策略从 First-Fit 改成 Best-Fit。** Best-Fit 与 First-Fit 共用同一套数据结构（按地址升序排列的空闲链表 + 用 `property` 记录块大小），因此初始化、释放合并、计数等逻辑都无需改动，两者的唯一区别在于"分配时选哪个块"。所以只改 `best_fit_alloc_pages` 的查找循环：不再"遇到第一个 `property >= n` 的块就 break"，而是遍历整条空闲链表，每当遇到一个 `property >= n` 的块就与当前记录的候选块比较，保留 `property` 更小的那个；遍历结束后，对选中的最小块执行切分、`nr_free` 减去 `n`、清除块头的 `PG_property` 标记并返回该块首地址。

**问题回答：你的 Best-Fit 算法是否有进一步的改进空间？**

Best-Fit 仍是 O(n) 线性扫描——每次分配都要遍历整条空闲链表才能确定「最小够用」的块，空闲块越多查找越慢；同时它倾向于切出更小的剩余块，容易在内存中积累大量极小碎片。改进方向：一是用按块大小排序的结构（如平衡树/红黑树，或按大小分级的多级链表/分离适配）把查找从 O(n) 降到 O(log n)；二是设置最小切分阈值，剩余块小于阈值时不再切分，以减少过小碎片的产生。

---

#### 任务三：实现 Buddy System（伙伴系统）分配算法

**负责人：** 2412796-杨斯源（组长）

**说明：** 实验**不提供任何代码框架和测试函数**，需要自行分析设计模块与验证方案，并**提交两份 prompt**。

**需要交付：**

| 交付物 | 内容 |
|---|---|
| 分配模块 | 从零生成的 Buddy System 模块，并接入现有内核（可正常编译、`make qemu` 可运行） |
| 测试函数 | 针对该实现自行设计的测试，检验分配与释放是否正确 |
| **两份 prompt** | 第一份：让 AI 参考现有 ucore 物理内存管理框架，从零生成 Buddy 模块并接入内核<br>第二份：让 AI 针对第一份生成的算法设计测试函数 |

**算法要点：**

- 可用存储空间划分为大小为 2 的 n 次幂的块（1, 2, 4, 8, 16, 32, 64, 128 …）
- 分配时向上取整到 2 的幂；若没有正好合适的块，就把更大的块**对半分裂**直到合适
- 释放时检查其**伙伴块**（与自己在同一对半区域内的另一半）是否也空闲，是则**合并**并递归向上
- 算法的时空复杂度不做要求，能通过测试即可

**最终提示词（两份）：**

<!-- 见 report/prompt.md；此处粘贴最终定稿 -->

**实现迭代过程：**

<!-- 记录 AI 生成 Buddy 实现时出现的 bug（如伙伴块地址计算错误、分裂/合并的循环边界、
     位运算求阶数出错等）以及解决过程 -->

**问题回答：测试覆盖了哪些核心行为和边界情况？这些测试为什么能有效检验生成的 Buddy 实现？**

<!-- 提示方向：核心行为——按 2 的幂分配、分裂、释放、伙伴合并、向上递归合并、连续分配耗尽；
     边界情况——请求非 2 的幂大小、请求超过最大块、申请 1 页、耗尽后再释放、反复分配释放不泄漏 -->

[待填写]

---

## 五、测试与验证

**编译与运行（`make qemu`）：**

<!-- 预期输出（实现 Best-Fit 并切换后）：
     Physical Memory from DTB: Base 0x80000000 / Size 128 MB / End 0x87ffffff
     (THU.CST) os is loading ...
     memory management: best_fit_pmm_manager
     物理内存探测输出
     check_alloc_page() succeeded!
     satp virtual address: 0xffffffffc0204000
     satp physical address: 0x0000000080204000 -->

![make qemu 运行结果](./images/lab2_qemu.png)

**自动评分（`make grade`）：**

<!-- lab2 起课程提供自动评分：grade.sh 会把整棵树复制到临时目录、用 inject.py 注入测试，
     再运行 QEMU 并在串口输出里匹配两条结果，各 10 分共 20 分：
       [Test 1] Page Init: PASS      （page_init 的 8 条验收）
       [Test 2] Best-Fit: PASS       （pmm_manager 已切换为 best_fit_pmm_manager）
     截取 Total Score: 20/20 的输出 -->

![make grade 评分结果](./images/lab2_grade.png)

---

## 六、实验总结与收获

### 对操作系统的理解

<!-- 要求：
     1. 列出本实验中重要的知识点，与对应 OS 原理中的知识点，简要说明二者的含义、关系和差异
     2. 列出 OS 原理中很重要、但在本实验中没有对应上的知识点
     可参考的方向：物理内存探测与设备树、页描述符与物理页的"身份证"、
     连续物理内存分配（First-Fit / Best-Fit / Buddy）、内部碎片与外部碎片、
     Sv39 分页与页表项（PTE）、地址转换（虚拟↔物理）、satp 寄存器与 TLB -->

**1. 本实验中重要的知识点，及与 OS 原理中对应知识点的关系**

[待填写]

**2. OS 原理中很重要、但本实验没有对应上的知识点**

[待填写]

### AI 协作开发的经验

<!-- 要求：结合本实验真实经过写。素材方向：
     - 本仓库对课程代码做的环境适配（-device loader 在 QEMU ≥ 8 下失效，改用 -kernel）
     - 从零生成 Buddy 模块时 AI 第一次给出的实现在哪些地方不对
     - 设备树信息的解析与地址转换（虚拟/物理/页号）容易混淆之处
     - 用 make grade 的断言反推实现错误的定位过程 -->

[待填写]

---
