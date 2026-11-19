# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 4: 进程管理（虚拟内存页操作 / 内核线程 / 调度） |
| **小组成员** | 2412796-杨斯源（组长）、2413628-黄乐仁、2410868-孙锘梵 |
| **完成日期** | 2026-11-19 |

### 小组分工

练习如何分工？

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2412796-杨斯源（组长） | 部分三：进程调度（`cpu_idle` / `schedule` / `proc_run` / `switch_to`）+ 收口与验证 |
| 2413628-黄乐仁 | 部分一：虚拟内存页操作（`page_insert` / `page_remove`）+ 补回 lab2 / lab3 的代码 |
| 2410868-孙锘梵 | 部分二：进程创建及初始化（`alloc_proc` / `proc_init` / `kernel_thread` / `do_fork` / `copy_thread` / `copy_mm`） |

实验报告如何分工？

报告的撰写按上表划分：各功能模块由其负责人撰写对应章节；实验环境、总结与收获及 `prompt.md` 的汇总由部分三负责人负责；最后经统一校对格式与图片引用后提交。

---

## 一、实验目的

本实验的主要目的是：

1. **掌握虚拟内存页的管理接口**：在多级页表框架上实现 `page_insert()` 与 `page_remove()`，理解"页表项内容、物理页引用计数、TLB 状态"三者必须同步。
2. **理解进程控制块与内核线程**：掌握 `struct proc_struct` 各字段的含义，理解 `idleproc` 与 `initproc` 的创建方式差异，以及"复制现有进程"这一创建模型。
3. **掌握线程首次运行的上下文构造**：理解 `kernel_thread → do_fork → copy_thread → forkret → forkrets → __trapret` 这条链路如何让新线程从指定函数入口开始执行。
4. **理解上下文切换与调度**：掌握 `struct context` 为什么只保存被调用者保存寄存器，理解 `switch_to` 的寄存器交换、`proc_run` 的页表切换与 `schedule` 的选线程策略。

---

## 二、实验环境

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| 2412796-杨斯源 | Claude Code（终端 Agent） | DeepSeek V4 Flash | |
| 2413628-黄乐仁 | Claude Code（终端 Agent） | DeepSeek V4 Flash | |
| 2410868-孙锘梵 | DeepSeek Harness（终端/Web Agent） | deepseek-flash | |

**运行环境：**

| 组件 | 版本 / 包名 |
|------|-------------|
| 操作系统 | Ubuntu 24.04.5 LTS（WSL2） |
| 交叉编译器 | `riscv64-unknown-elf-gcc` 15.1.0 |
| 二进制工具 | `riscv64-unknown-elf-ld` / `objcopy` / `objdump` |
| 模拟器 | `qemu-system-riscv64` 8.2.2（apt 包 `qemu-system-misc`，自带 OpenSBI v1.3） |
| 调试器 | `gdb-multiarch` 15.1 |

**本实验做的环境适配**（与 lab2/lab3 同源，已提交在 `code/` 内）：

| 位置 | 改动 | 原因 |
|---|---|---|
| `Makefile`、`tools/grade.sh` | `-device loader,file=...,addr=0x80200000` → `-kernel bin/ucore.img` | QEMU ≥ 8 自带 OpenSBI 为 fw_dynamic，跳转地址必须由 QEMU 告知，否则内核不执行 |
| `kern/init/entry.S` | 启动页表补回低 1G 恒等映射 | 打开分页瞬间 PC 仍在物理地址 `0x8020xxxx`，不映射会立即取指页错误 |
| `kern/mm/default_pmm.c` | 删除不存在的 `#include <best_fit_pmm.h>` | 工程中无此头文件，保留则编译失败 |
| `kern/mm/kmalloc.c` | `kva2page(kva)` → `kva2page((void *)kva)` | 新版 GCC 下 `-Wint-conversion` 直接报错 |
| `Makefile` | GDB 命令自动探测 `riscv64-unknown-elf-gdb` / `gdb-multiarch` | apt 仓库没有前者 |
| `tools/grade.sh` | 不设 GDB 断点，跑满固定时间后收串口日志 | 原脚本的断点地址按老平台约定推导，在 riscv64 上不可用 |
| `kern/mm/pmm.c` | `get_pte()` 的中间级页表项只置 `PTE_V`（去掉 `PTE_U`）；`check_pgdir()` 的 `boot_pgdir_va[0] & PTE_U` 断言改为 `PTE_V` | RISC-V 规定非叶子 PTE 的 U 位为保留位必须为 0，QEMU 8.2 在地址翻译时严格执行；否则 `check_boot_pgdir()` 首次访问映射地址即 store page fault |

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

本章要解决的问题是：**让内核从"一条执行流跑到底"变成"可以创建、也可以调度的多条执行流"**。为此需要依次补齐三样东西——地址空间、执行实体、切换机制。

**第一条线索是地址空间。** lab2 只在内核页表里建了一张粗糙的 gigapage 映射，能让内核跑起来，却没有"把某个物理页挂到某个线性地址上"的可复用操作。本章先实现 `page_insert()` / `page_remove()`，把映射的建立与撤销、物理页引用计数、TLB 失效三件事捏成一个可靠的接口；再由 `vmm.c` 用 `mm_struct` + `vma_struct` 描述一段地址空间。本阶段的虚拟内存是**预映射**的：建立时一次性完成映射，不涉及缺页异常与页面置换。

**第二条线索是执行实体。** 用 `struct proc_struct` 描述一个线程：状态、pid、内核栈、页目录、上下文、中断帧。两种创建方式都要实现——`idleproc` **复用**内核启动以来的执行现场（所以它的 `kstack` 就是 `bootstack`、`tf` 为 `NULL`），`initproc` 则走完整的"复制现有线程"路径：`kernel_thread()` 手工构造一个 trapframe，`do_fork()` 分配 PCB 与内核栈，`copy_thread()` 把这个 trapframe 安放到新栈顶并规定首次运行入口。

**第三条线索是切换。** 有了对象还要让 CPU 真正切过去：`schedule()` 从就绪链表中选下一个线程，`proc_run()` 更新 `current`、把 `satp` 指向新线程的页目录，`switch_to()` 交换那 14 个被调用者保存寄存器。新线程第一次被切进来时，`context.ra` 已被设成 `forkret`，于是执行流会沿 `forkret → forkrets → __trapret → sret` 走回那个手工构造的 trapframe，最终从 `kernel_thread_entry` 进入用户指定的函数。

把三条线索串起来，就是本章的启动主链：

```
kern_init → pmm_init（页表池 + check_pgdir）→ vmm_init
        → proc_init（idleproc / initproc）→ clock_init → intr_enable
        → cpu_idle → schedule → proc_run → switch_to
        → forkret → forkrets → __trapret → sret → kernel_thread_entry → init_main
```

前半段准备地址空间与进程管理结构，后半段让第一个真正创建出来的线程接管 CPU。

### 3.2 功能的逐步实现

本章按如下顺序逐步实现，每一步都为下一步提供前提：

1. **先把 lab2 / lab3 的代码补回来。**
   → 为什么最先做？本实验依赖前两次实验，代码里留着四处 `//lab2` / `//lab3` 标好的空缺；不补则编译、链接都过不去，后面的功能一行都验证不了。

2. **实现页表维护接口**（`page_insert()` / `page_remove()`）。
   → 为什么紧接着做？`pmm_init()` 内部的 `check_pgdir()` 会直接调用这两个函数，接口不对连启动自检都过不了；同时它们决定了"一个物理页被多少个地址引用"的语义，是后面所有地址空间操作的基础。

3. **建立地址空间描述**（`vmm_init()` + `mm_struct` / `vma_struct`）。
   → 为什么排在这里？进程需要"自己的地址空间"这个概念，但本阶段是预映射，所以只需把 VMA 的创建、按地址排序插入、查找做对；真正的缺页处理留给后续实验。

4. **设计 PCB 并实现 `alloc_proc()`**。
   → 为什么先要对象？调度器操作的最小单位是 PCB，没有干净的 PCB 就无从谈"选择"与"切换"。第一步必须把字段清零再赋安全初值，否则 `kmalloc` 回来的垃圾值会让后面每一条断言都失败。

5. **创建 `idleproc` 与 `initproc`**（`proc_init()` / `kernel_thread()` / `do_fork()` / `copy_thread()` / `copy_mm()`）。
   → 为什么两种方式都要有？`idleproc` 代表"内核启动现场本身"，直接复用当前栈与现场；`initproc` 代表"由现有线程复制出来的新线程"，需要独立的 PCB、内核栈和首次运行入口。后者是后续实验中创建用户进程的通用模型。

6. **实现调度与上下文切换**（`schedule()` / `proc_run()` / `switch_to()` / `cpu_idle()`）。
   → 为什么最后做？前五步只是在内存里"摆好"了线程对象，还没有任何代码把 CPU 交给它们。`switch_to` 只交换寄存器，`proc_run` 负责页表与 `current`，`schedule` 负责选谁——三层职责分开，任何一层缺少，验证都无法进行。

7. **验证**：三个测试分别注入在 `pmm_init()` 末尾、`proc_init()` 末尾、`init_main()` 开头，正好对应"页表接口正确 → 线程对象正确 → 线程真的被调度运行"这三个层次。
   → 为什么这样安排？它让失败点自然收敛：Test PMM 挂了说明第 2 步有问题，Test Process Create 挂了说明第 4、5 步有问题，Test Schedule 挂了才轮到第 6 步——而它同时也反过来检验第 5 步的 `kernel_thread()` 是否构造正确。

---

## 四、实验内容与实现

### 前置：补回 lab2 与 lab3 的代码

**负责人：** 2413628-黄乐仁、2410868-孙锘梵

实验指导书要求"使用此前设计的提示词生成相应实验代码，并填写到标有 lab2/lab3 注释的位置"。本实验代码中有以下四处：

| 位置 | 需要补回的内容 | 建议负责人 |
|---|---|---|
| `kern/mm/default_pmm.c`（第 6 行附近） | lab2 的 Best-Fit 管理模块（`free_area` + 五个管理函数 + 管理器注册） | 孙锘梵（lab2 写过 Best-Fit） |
| `kern/mm/pmm.c`（第 90 行附近） | lab2 的 `page_init()` | 黄乐仁（lab2 写过 `page_init`） |
| `kern/trap/trap.c`（第 86 行附近） | lab3 的 `interrupt_handler()` / `exception_handler()` | 孙锘梵（lab3 写过两个 handler） |
| `kern/trap/trapentry.S`（第 56、63 行附近） | lab3 的 `RESTORE_ALL` 与 `__alltraps` 调用部分 | 黄乐仁（lab3 写过 `RESTORE_ALL`） |

> **注意**：管理器结构体名在本实验中是 `default_pmm_manager`（见 `kern/mm/default_pmm.h`），
> 但内部函数仍叫 `best_fit_*`、测试函数仍叫 `best_fit_check()`；补回 lab2 代码时只需改名、保留逻辑。
> `basic_check()` 与 `best_fit_check()` 由框架提供，**不得修改或重复实现**。

**补回后的验证：** `make` 能通过、`make qemu` 能启动到 `vmm_init` 的页表检查。

**Best-Fit 补回说明（2410868-孙锘梵）：**

lab4 把 lab2 的 `best_fit_pmm.c` 并进了 `kern/mm/default_pmm.c`，而且**没有** `best_fit_pmm.h`。所以不能把 lab2 的整个文件复制过来：那会重复定义 `basic_check()` 与 `best_fit_check()`，还会多出一个 lab4 根本不引用的 `best_fit_pmm_manager`。补回时只补三部分：

| 补回内容 | 说明 |
|---|---|
| 空闲区结构 | `static free_area_t free_area;`，再用两个宏把 `free_list` / `nr_free` 映射到它的成员；文件里已提供的两个测试函数正是通过这两个名字访问空闲链表，缺了直接编译失败 |
| 五个管理函数 | `best_fit_init`、`best_fit_init_memmap`、`best_fit_alloc_pages`、`best_fit_free_pages`、`best_fit_nr_free_pages`，逻辑与 lab2 完全一致 |
| 管理器注册 | `const struct pmm_manager default_pmm_manager`，`.name = "default_pmm_manager"`、`.check = best_fit_check`；`pmm.c` 第 40 行正是按这个名字取管理器 |

与 First-Fit 的实质差别仍然只在 `best_fit_alloc_pages()` 的选块策略：First-Fit 遇到第一个 `property >= n` 的块就返回；Best-Fit 遍历整条空闲链表，在**所有**满足 `property >= n` 的块里挑 `property` 最小的那一块，把本次分配切剩下的碎片压到最小。释放时则按地址顺序插回链表，并把前、后相邻的空闲块合并，保证空闲链表始终按地址有序。

最终提示词见 [`report/prompt.md`](./prompt.md) 第二节「提示词 0」。

**Trap 两个 handler 补回说明（2410868-孙锘梵）：**

lab4 的 `trap.c` 删掉了 lab3 的 `trap_dispatch()` 与 `trap_in_kernel()`，改成在 `trap()` 末尾直接分流：

```c
void trap(struct trapframe *tf)
{
    if ((intptr_t)tf->cause < 0)
        interrupt_handler(tf);   // 中断
    else
        exception_handler(tf);   // 异常
}
```

这带来一个与 lab3 不同的约束：两个 handler **必须定义在 `trap()` 之前**（骨架的标记处正好在文件中部），否则编译器先见到调用、后见到定义，会报 `implicit declaration of function` 而无法编译。其余逻辑与 lab3 一致——`interrupt_handler` 先剥离 `scause` 最高位再分流，时钟分支必须先 `clock_set_next_event()` 再 `ticks++`，否则时钟只会来一次；`exception_handler` 对非法指令与断点按指令编码推进 `epc`（读 `epc` 处 16 位字，低两位 `0b11` 为 4 字节，否则为 2 字节压缩指令），不能写死 `epc += 4`——lab3 就是在这里踩过 `c.ebreak` 只有 2 字节的坑。

**`page_init()` 补回说明（2413628-黄乐仁）：**

lab4 的 `page_init()` 与 lab2 同构，但它必须在 `pmm_init()` 的 `check_alloc_page()` 之前跑完：`check_pgdir()` 会直接用 `alloc_page()` 申请页表页，依赖 `page_init()` 已经设置好 `va_pa_offset`、`npage`、`pages` 并把剩余内存交给 `init_memmap()`。补回时保留 lab2 的七步：

1. `va_pa_offset = PHYSICAL_MEMORY_OFFSET`；
2. 从 DTB 取 `mem_base` / `mem_size`，无效则 panic；
3. `npage = PPN(mem_end)`；
4. `pages = (struct Page *)ROUNDUP((uintptr_t)end, PGSIZE)`，把 Page 描述符数组放在内核镜像之后；
5. 先把 `[nbase, npage)` 的全部页 `SetPageReserved`；
6. `freemem = PADDR((uintptr_t)pages + sizeof(struct Page) * npage)`；
7. 把 `[freemem, mem_end)` 按页对齐后交给 `init_memmap()`。

关键点是两套地址空间不能混用：`pages` 是内核虚拟地址，而 `freemem` 必须先经 `PADDR()` 转成物理地址再传给 `init_memmap()`；另外必须先全部置 reserved、再释放数组之后的内存，反过来会把内核镜像和 Page 数组本身也交给分配器。

**`RESTORE_ALL` / `__alltraps` 补回说明（2413628-黄乐仁）：**

lab4 的 `SAVE_ALL` 与 lab3 只有一处差别：lab4 用 `csrr s0, sscratch` 保存原 `sp`，lab3 用 `csrrw s0, sscratch, x0` 并在同一条指令里清零。因此 `RESTORE_ALL` 的恢复顺序与 lab3 一致——先 `csrw sepc`、`csrw sstatus`，再按编号恢复 `x1`、`x3`~`x31`（`x0` 硬连线为 0，不恢复），最后从 trapframe 的 2 号槽恢复 `x2 (sp)`。`sp` 必须最后恢复，否则后面所有 `LOAD ... sp` 都会读到错误位置。`__alltraps` 在 `SAVE_ALL` 之后把 trapframe 指针作为第一个参数传给 `trap()`：`mv a0, sp; jal trap`；`__trapret: RESTORE_ALL; sret` 与 `forkrets: move sp, a0; j __trapret` 保持骨架原样。首次运行新线程时，`copy_thread()` 把 `context.sp` 指向栈顶的 trapframe，`switch_to` 返回 `forkret` 后再跳到 `forkrets`，最后正是由这段 `RESTORE_ALL` 把 `sp` 恢复到 trapframe 并 `sret` 到 `kernel_thread_entry`。

---

### 功能模块一：虚拟内存页操作功能

**负责人：** 2413628-黄乐仁

**需要实现的函数（`kern/mm/pmm.c`）：**

```c
int  page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm);
void page_remove(pde_t *pgdir, uintptr_t la);
```

**功能说明：**

`page_insert(pgdir, page, la, perm)` 负责把一个物理页映射到线性地址 `la`。它先用 `get_pte(pgdir, la, 1)` 找到末级页表项（中间级页表不存在时按需分配），分配失败返回 `-E_NO_MEM`；随后区分三种情况：地址未映射、已映射到同一物理页、已映射到其他物理页。第一种直接建立映射；第二种只改权限，引用计数净增 0；第三种先撤销旧页的映射（旧页引用计数减一，减到 0 才释放），再写入新页。最后写 `pte_create(page2ppn(page), perm)` 并 `tlb_invalidate()`。

`page_remove(pgdir, la)` 用 `get_pte(pgdir, la, 0)` 查找页表项：中间级页表从未建立时安静返回；页表项无效时只刷新 TLB；有效时调用 `page_remove_pte()` 撤销映射。辅助函数 `page_remove_pte()` 把"页表项清零、引用计数减一、必要时释放物理页、刷新 TLB"这四件必须同步的事情集中在一处，`page_insert()` 撤销旧映射时也复用它。

本模块的两个函数本身只维护映射关系，不负责页表页的创建与回收；中间级页表页的生命周期仍由 `get_pte()` 与测试代码管理。另有一处与功能实现相关的环境修复：`get_pte()` 创建中间级页表项时只置 `PTE_V` 而不置 `PTE_U`（详见「实现迭代过程」）。

**注意事项（指导书原文要点）：**

- `page_insert` 不能简单覆盖旧页表项，要区分"原地址已映射到同一物理页"与"已映射到其他物理页"两种情况，分别处理引用计数。
- `get_pte(pgdir, la, 1)` 可能分配中间级页表页，**分配失败必须返回 `-E_NO_MEM` 且不留下半完成映射**。
- `page_remove` 要能处理"目标地址本来就没有映射"的情形，安静返回。
- 所有页表更新后都要 `tlb_invalidate()`，否则测试中会出现"页表已改、翻译还是旧的"。

**验收标准：**

- `[Test PMM] PASS` —— 10 分。

**最终提示词：** 定稿原文如下（与 [`report/prompt.md`](./prompt.md) 第一节「提示词 2」一致）。

````markdown
[PROMPT]

**任务**：修改 `code/kern/mm/pmm.c`，在 `//LAB4 function1 YOUR CODE` 标记处实现 `page_insert()` 与 `page_remove()`，可以额外增加一个 `static void page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep)` 辅助函数。

**操作要求**：必须在项目中修改实际文件，而不是只展示代码。不要修改 `check_alloc_page()`、`check_pgdir()`、`check_boot_pgdir()` 的测试逻辑（除下文要求的一处端口断言外）；保留 `//LAB4 function1` 注释并把 `YOUR CODE` 换成本人学号。

**环境约束**：本工程运行在 QEMU 8.2 的 Sv39 上，MMU 会严格执行 RISC-V 规范中"非叶子页表项的 U 位必须为 0"的规定。`get_pte()` 创建中间级页表项时只能置 `PTE_V`，不能置 `PTE_U`；`check_pgdir()` 里从 x86 移植来的 `assert(boot_pgdir_va[0] & PTE_U)` 应改为检查 `PTE_V`。否则 `check_boot_pgdir()` 第一次真正访问映射地址时会在地址翻译阶段触发 store page fault。

**输出要求**：直接进行文件操作，使用下面 [RELY]、[GUARANTEE]、[SPECIFICATION] 的信息，以工程真实代码为准。

[RELY]

```c
pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create);
struct Page *get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store);
void tlb_invalidate(pde_t *pgdir, uintptr_t la);
struct Page *pte2page(pte_t pte);
ppn_t page2ppn(struct Page *page);
uintptr_t page2pa(struct Page *page);
pte_t pte_create(uintptr_t ppn, int type);   // 返回值已经包含 PTE_V
int page_ref_inc(struct Page *page);
int page_ref_dec(struct Page *page);         // 返回减少后的引用计数
int page_ref(struct Page *page);
#define free_page(page) free_pages(page, 1)

#define PTE_V 0x001
#define PTE_R 0x002
#define PTE_W 0x004
#define PTE_X 0x008
#define PTE_U 0x010
#define E_NO_MEM 4
```

调用方约束（来自 `check_pgdir()` / `check_boot_pgdir()` / `test_lab4_pmm_basic()`）：
- `perm` 可以是 0（只建立映射，不给 R/W）；
- 同一物理页可以重复 `page_insert()` 到同一地址，只改权限；
- 同一地址可以从一个物理页改映射到另一个物理页；
- `page_remove()` 可以对同一地址连续调用，也可以对从未映射的地址调用。

[GUARANTEE]

```c
int  page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm);
void page_remove(pde_t *pgdir, uintptr_t la);

// 可选但推荐
static void page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep);
```

[SPECIFICATION]

## page_remove_pte

**Pre-Condition**：
- `ptep` 是 `pgdir` 中对应 `la` 的末级页表项指针，且 `*ptep & PTE_V`。

**Post-Condition**：
- 原物理页的引用计数减 1；减到 0 时调用 `free_page()`；`*ptep == 0`；对应 TLB 项已失效。

**Case 1（引用计数仍大于 0）**：
- 只减引用并清空页表项，不释放物理页。

**Case 2（最后一个引用）**：
- `page_ref_dec(page) == 0` 时 `free_page(page)`。

**Requirements**：
- 页表项内容、引用计数、TLB 三者必须同步；清空 `*ptep` 不能漏。

## page_insert

**Pre-Condition**：
- `pgdir` 是页目录内核虚拟地址；`page` 是合法 Page 描述符；`la` 任意；`perm` 是要写入的权限位（不含 `PTE_V`）。

**Post-Condition**：
- 成功返回 0，`la` 对应的页表项为 `pte_create(page2ppn(page), perm)`，`page` 的引用计数因这次映射净增 1（改映射同一页时净增 0），TLB 已刷新。
- 中间级页表分配失败时返回 `-E_NO_MEM`，且不留下半完成映射。

**Case 1（`get_pte(..., 1)` 返回 NULL）**：
- 直接返回 `-E_NO_MEM`。

**Case 2（原地址未映射）**：
- 新页引用计数 +1，写入页表项。

**Case 3（原地址已映射到同一物理页）**：
- 只改权限。先按新映射 +1，再回退这次 +1，使净增为 0；`nr_free_pages()` 不变化。

**Case 4（原地址映射到其他物理页）**：
- 新页引用计数 +1，然后对旧页调用 `page_remove_pte()` 撤销旧映射（旧页引用计数 -1，必要时释放）。

**Requirements**：
- 必须先 `page_ref_inc(page)` 再处理旧项，否则 Case 3/4 的回退顺序容易把计数弄错。
- 最后写 `*ptep = pte_create(...)` 并 `tlb_invalidate(pgdir, la)`。
- `pte_create()` 内部已经 OR 了 `PTE_V`，不要再重复加。

## page_remove

**Pre-Condition**：
- `la` 可以已映射、未映射，或从未建立过页表。

**Post-Condition**：
- 若 `la` 有有效映射则撤销；否则不做任何修改。函数无返回值，可重复调用。

**Case 1（`get_pte(..., 0)` 返回 NULL）**：
- 安静返回。

**Case 2（页表项存在但无效）**：
- 只需 `tlb_invalidate()`（可选），不要解引用页框。

**Case 3（有效映射）**：
- 调用 `page_remove_pte()`。
````

**实现迭代过程：**

**第一轮：引用计数与空指针。** 最初按 x86 版思路直接 `page_ref_inc(page)` 后覆盖页表项，没有区分旧映射。结果同一物理页重复映射时引用计数多 1，旧物理页也释放不掉，`check_pgdir()` 在 `page_ref(p2) == 0` 处断言失败。`page_remove()` 初版还直接解引用 `get_pte()` 的返回值，对未映射地址会解引用 NULL。改为"先 inc 新页，再判断旧项"：旧项有效且 `old == page` 时回退这次 inc；`old != page` 时调用 `page_remove_pte()` 撤销旧映射；`page_remove()` 先判 `get_pte(..., 0)` 是否为 NULL，再判 `PTE_V`。修改后 `check_pgdir()` 通过。

**第二轮：`check_pgdir()` 之后的"二次启动"。** 加入 PMM 测试后执行 `make grade`，串口在 `check_pgdir() succeeded!` 之后直接重启：先又打印一遍 `DTB Init`、`HartID: 256`，最后在 `page_init()` 里因 DTB 内存信息为 0 而 panic。起初怀疑是 `page_insert()` 的引用计数或页表项写错，但用 GDB 在 `strcpy` 处断下并手工读取页表链，`satp` 指向的 root → L1 → L0 每一级 PTE 都是 `V/R/W` 齐全的有效项，与 `page_insert()` 的预期完全一致。

进一步用 `qemu -d int` 抓到唯一一条非 SBI 的异常：`store_page_fault, epc=strcpy, tval=0x100`。`0x100` 正是 `check_boot_pgdir()` 映射后要写的地址，说明页表项本身有效、但 MMU 在地址翻译阶段拒绝了这次访问。做单变量对照实验：手工在 root 级建立同一映射，页表项 `U=0` 时访问正常，`U=1` 时就 fault。对照 RISC-V 规范后确认：**非叶子页表项的 U 位是保留位，必须为 0**；而骨架 `get_pte()` 给两级中间页表项都置了 `PTE_U | PTE_V`。此前的 `check_pgdir()` 只通过 `get_pte()` 读写页表内容，从不让 CPU 真正访问映射地址，所以一直没暴露；`check_boot_pgdir()` 是第一次真正解引用。此时 `idt_init()` 尚未执行，`stvec` 仍是固件留下的 `0x80200000`（低地址恒等映射的内核入口），于是 store page fault 被当成重新进入内核，表现出来就是"二次启动"。

修复分两处：`get_pte()` 的中间级页表项只置 `PTE_V`；`check_pgdir()` 里从 x86 移植来的 `assert(boot_pgdir_va[0] & PTE_U)` 改为检查 `PTE_V`。修复后 `check_pgdir()`、`check_boot_pgdir()` 都打印 succeeded，`[Test PMM] PASS`。这属于 QEMU 8.2 严格实现 Sv39 带来的端口问题，不影响 `page_insert()` / `page_remove()` 的接口语义。

---

### 功能模块二：进程创建及初始化功能

**负责人：** 2410868-孙锘梵

**需要实现的函数（`kern/process/proc.c`）：**

```c
static struct proc_struct *alloc_proc(void);
void proc_init(void);
int  kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags);
int  do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);
static void copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf);
static int  copy_mm(uint32_t clone_flags, struct proc_struct *proc);
```

**功能说明：**

本模块负责把内核从「只有一条启动执行流」推进到「有 PCB、能创建内核线程」。六个函数是同一条创建链路上的不同环节：

- `alloc_proc()` 负责 PCB 的**分配与初值**。它从内核堆取一块 `struct proc_struct`，整体清零后只设置少数几个有意义的字段（`state = PROC_UNINIT`、`pid = -1`、`pgdir = boot_pgdir_pa`、`need_resched = 0`），把「未初始化」做成一个可检查的确定状态，而不是让字段停在堆上的随机值。
- `proc_init()` 负责**系统启动时的两条特殊线程**。`idleproc` 直接复用当前启动栈（`bootstack`），pid 固定为 0，随后把 `current` 指向它——此后任何 `do_fork` 都能把 `current` 当作父进程；`initproc` 则通过 `kernel_thread(init_main, "Hello world!!", 0)` 走一遍完整的 fork 流程创建。
- `kernel_thread()` 与 `do_fork()` 负责**通用创建路径**。前者把「入口函数 + 参数」包装成一个 trapframe，并把 `epc` 指向汇编入口 `kernel_thread_entry`；后者完成 PCB、内核栈、地址空间三步资源分配，失败时按逆序回滚，成功后把新 PCB 挂进 `proc_list` 与哈希表并置为可运行。
- `copy_thread()` 负责**首次运行的上下文**。它在内核栈顶伪造一份 trapframe 副本，并把 `struct context` 的 `ra` 设成 `forkret`、`sp` 指向这份 trapframe；新线程第一次被 `switch_to` 换上来时从 `forkret` 进入 `__trapret`，`RESTORE_ALL` 恢复寄存器后 `sret` 跳到 `tf->epc`，于是「第一次运行」看起来就像一次从中断返回。
- `copy_mm()` 在本阶段只做**占位**：所有进程都是内核线程，`mm` 保持 `NULL`。

这条链路的关键在于：新线程的第一次执行不是函数调用，而是一次精心构造的**返回**。`context.ra` 决定切换后先到哪里，`tf->epc` 决定 `sret` 之后到哪里，`tf->gpr.s0/s1` 决定 `kernel_thread_entry` 拿到什么；三者少一个或写错一个，线程要么跑飞，要么在第一次压栈时就崩。

**注意事项（指导书原文要点）：**

- `alloc_proc` 要把 PCB 置于安全的空白状态：**整块清零**再设置少量字段。测试会断言 `tf == NULL`、`parent == NULL`、`mm == NULL`、`runs == 0`、`flags == 0`、`context.ra == 0`，只设置自己关心的字段必然漏项。
- `proc_init` 中 `idleproc` **不是 fork 出来的**，而是直接复用当前执行现场（`bootstack`、`pid = 0`、`need_resched = 1`，`tf` 与 `context.ra` 保持 0）；因此它绝不能走 `kernel_thread`。
- `current = idleproc` 必须在 `kernel_thread(init_main, ...)` **之前**完成；`do_fork` 里的 `proc->parent = current` 与 `copy_mm` 里的 `current->mm` 都依赖它。
- `kernel_thread` 与 `copy_thread` 必须配合 `forkret → forkrets → __trapret` 形成正确的首次运行链路：`context.ra = forkret`、`tf->epc = kernel_thread_entry`、`tf->gpr.s0 = fn`、`tf->gpr.s1 = arg`，且 `context.sp` 指向 `proc->tf`。
- `copy_thread` 中 `esp == 0`（内核线程）时，`tf->gpr.sp` 必须回退到 `(uintptr_t)proc->tf`；若照抄成 `0`，线程第一次压栈就会访问非法地址。
- `do_fork` 的前三步（PCB、内核栈、mm）失败时必须按「内核栈 → PCB」逆序回滚；pid 分配、入哈希表、入进程链表必须在同一个 `local_intr_save/restore` 临界区内完成，成功后 `wakeup_proc`。
- 本阶段 `copy_mm` 只做占位：内核线程 `mm` 为 `NULL`，不做真正的地址空间复制，也不要改 `pgdir`。

**验收标准：**

- `[Test Process Create] PASS` —— 10 分（检查 `idleproc` 的 pid/state/kstack/parent/mm/tf/runs/flags/context，以及 `initproc` 的 pid/parent/context.ra/sp）。

**最终提示词：**

定稿原文如下，全文同时收录于 [`report/prompt.md`](./prompt.md) 第二节「提示词 2」。

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

**实现迭代过程：**

**一、先把测试断言翻译成约束**

`tools/test_lab4.c` 的 `test_lab4_process_create()` 是在 `proc_init()` 返回后立刻执行的，逐字段检查 `idleproc` 与 `initproc`。这一步实际上决定了实现方式：`idleproc->tf == NULL` 与 `context.ra == 0` 意味着 idleproc 不能走 fork；`initproc->parent == idleproc` 意味着 `current` 必须先赋值；`initproc->context.ra != 0`、`context.sp != 0` 意味着 `copy_thread` 必须真的把 `context` 填起来。六个函数还没动笔，这些断言已经把「哪些字段必须是什么」定死了。

**二、按创建链路的顺序分步实现**

1. `alloc_proc`：`kmalloc` + `memset` 整块清零，再设四个字段；单独编译 `make obj/kern/process/proc.o` 确认无警告。
2. `copy_mm`、`copy_thread`：先把被 `do_fork` 调用的两个环节写好——一个决定地址空间，一个决定首次上下文。
3. `kernel_thread`：只构造 trapframe，不直接调用 `fn(arg)`，统一交给 `do_fork` 走调度器。
4. `do_fork`：资源分配 → 失败回滚标签 → 临界区内入表 → `wakeup_proc`。
5. `proc_init`：最后写，因为它依赖前面所有函数；先建 idleproc 并赋 `current`，再 `kernel_thread` 建 initproc。

**三、四个必须写死的细节**

1. **PCB 必须整块清零。** 逐字段赋值时，`tf`、`parent`、`mm`、`runs`、`flags`、`context` 这些「暂时用不到」的字段会保留 `kmalloc` 返回的堆内容，而测试恰好逐个断言它们为 0 / NULL。用 `memset(proc, 0, sizeof(*proc))` 一次解决，也顺带保证 `list_link` / `hash_link` 处于可安全 `list_add` 的状态（两个指针都指向自己）。
2. **`current` 必须先于 `kernel_thread`。** `do_fork` 里要写 `proc->parent = current`，`copy_mm` 里要读 `current->mm`；若先创建 initproc 再赋 `current`，轻则父进程为空，重则解引用空指针。
3. **`context.ra` 是 `forkret`，不是 `kernel_thread_entry`。** `switch_to` 只恢复 `context` 里的 14 个寄存器，`s0` / `s1` 并不会被自动设成 `fn` / `arg`；真正的入口靠 `forkret → forkrets` 把 `sp` 切到 `proc->tf`，再由 `RESTORE_ALL` 从 trapframe 恢复 `s0` / `s1`，最后 `sret` 跳到 `tf->epc`。把 `ra` 写成 `kernel_thread_entry` 会跳过整段 trapframe 恢复，线程拿到的是切换前的残留寄存器值。
4. **`esp == 0` 时 `gpr.sp` 必须回退到内核栈顶。** 内核线程没有用户栈，`copy_thread` 收到的 `esp` 是 0；若直接写 `tf->gpr.sp = esp`，`RESTORE_ALL` 会把 `sp` 恢复成 0，`sret` 之后第一次压栈就写向地址 0。正确做法是 `gpr.sp = (uintptr_t)proc->tf`，让它从自己的 trapframe 顶部开始压栈。

**四、一次真实的弯路：把 lab3 的文件整个搬过来**

补回 Best-Fit 时最直接的想法是「lab3 的 `best_fit_pmm.c` 已经是一份完整实现，整体复制成 `default_pmm.c` 就行」。实际编译立刻失败，而且两侧都会失败：

- **带着测试函数搬**：lab4 的 `default_pmm.c` 里已经有框架提供的 `basic_check()` 与 `best_fit_check()`，再带一份就是重复定义；
- **只搬管理器**：lab4 没有 `best_fit_pmm.h`，`pmm.c` 只认 `default_pmm_manager`，搬过来的 `best_fit_pmm_manager` 没有任何地方引用；而如果把测试函数连同宏一起删掉，`free_list` / `nr_free` 又会立刻变成 `undeclared`——这正是骨架原状下 `default_pmm.c` 的编译错误。

最后的做法是「只搬中间那一段」：保留 lab4 的两个测试函数不动，只插入 `free_area` + 两个宏 + 五个管理函数，再把管理器结构体改名为 `default_pmm_manager`、`.check` 指向框架的 `best_fit_check`。补回 lab3 的两个 handler 时同理：lab3 的 `trap_dispatch()` 与 `trap_in_kernel()` 在 lab4 已经被删掉，照搬会多出两个无人调用的函数；而 handler 若按 lab3 的位置写在 `trap()` 之后，编译器先见到调用后见到定义，直接报 `implicit declaration of function 'interrupt_handler'`——这条错误在补回之前的基线构建里就能复现。

**教训**：补回旧代码不是文件级复制，而是接口级对齐。先看新骨架已经提供了什么（测试函数、头文件、管理器名字、调用顺序），再决定搬哪几段；否则不是重复定义就是缺符号。

**五、阶段性验证与遗留**

`make obj/kern/process/proc.o` 单独编译通过（`-std=gnu99 -Wall -Wno-unused`，无警告）。在本模块单独完成时，全量 `make -k` 只剩 `kern/mm/pmm.c:149: implicit declaration of function 'page_init'`——那是部分一当时尚未补回的 `page_init()`，部分三的 `sched.c` / `switch.S` 也还是空的。因此本模块的运行时验证（`[Test Process Create] PASS`）要等三部分合并后统一做；在此之前，六个函数的正确性靠「逐条对照测试断言 + 手工推演首次运行链路」保证。三部分合并后 `make grade` 为 **30/30**，说明上述推演与最终结果一致。

---

### 功能模块三：进程调度

**负责人：** 2412796-杨斯源（组长）

**需要实现的函数：**

```c
void cpu_idle(void);                                  // kern/process/proc.c
void schedule(void);                                  // kern/schedule/sched.c
void proc_run(struct proc_struct *proc);              // kern/process/proc.c
void switch_to(struct context *from, struct context *to);  // kern/process/switch.S
```

**功能说明：**

本模块负责把"已经创建并登记好的线程对象"真正交给 CPU 执行，四层职责自上而下分得很清楚：

- `cpu_idle()` 是空闲线程的执行主体。`proc_init()` 结束后当前执行现场就是 `idleproc`，它把 `need_resched` 置 1，于是 `cpu_idle` 一进来就调用 `schedule()` 让出 CPU；之后它只在"没有别的线程可跑"时继续空转。
- `schedule()` 只负责**选**。它清掉 `current->need_resched`，在 `proc_list` 上环形遍历找出一个 `PROC_RUNNABLE` 的线程；起点按 ucore 的轮转语义选择：当前不是 `idleproc` 就从自己的下一个开始，当前是 `idleproc` 就从链表头开始，一圈都没找到就退回 `idleproc` 保底。选中后交给 `proc_run()`。
- `proc_run()` 只负责**换**。它更新全局 `current`、把 `satp` 指向新线程的页目录物理地址、`sfence.vma` 使 TLB 失效，然后调用 `switch_to()` 交换上下文。整个过程关中断，避免"`current` 已改、实际还在旧线程上跑"的中间状态被中断打断。
- `switch_to()` 只负责**存与取**。它把 `from` 的 `ra`、`sp`、`s0..s11` 共 14 个寄存器写进 `struct context`，再把 `to` 的这 14 个值装回寄存器。之所以只存这些，是因为切换发生在函数调用内部，调用者保存寄存器（`t0..t6`、`a0..a7`）由编译器在调用点自行保存恢复，只有被调用者保存寄存器需要手工处理。

**新线程第一次运行的关键**：`switch_to` 末尾的 `ret` 会跳到 `to->context.ra`。对 `initproc` 而言，`copy_thread()` 已把 `context.ra` 设成 `forkret`、`context.sp` 设成它的 trapframe 地址，于是执行流走向 `forkret() → forkrets(tf) → __trapret → RESTORE_ALL → sret`，最终从 `kernel_thread_entry` 取出 `s0`（函数地址）与 `s1`（参数）开始执行。也就是说，**调度代码本身不需要知道"第一次运行"这件事**，它只是老老实实做了一次普通的上下文交换。

**注意事项（指导书原文要点）：**

- `schedule` 与 `proc_run` 的临界区必须关中断（`local_intr_save/restore`），否则 `current`、页表与实际执行现场可能不一致。
- 选线程三种情形：当前不是 idleproc → 从下一个开始找（轮转）；当前是 idleproc → 从头找；都找不到 → 退回 idleproc。
- `proc_run` 要更新 `current`、`write_csr(satp, ...)` 切页表、`flush_tlb()`，再 `switch_to`。
- **`libs/riscv.h` 的 `lsatp()` 是 Sv32 遗留，无法表达 Sv39 的 satp，不可使用**；参照 `kern/mm/pmm.c` 的 `enable_paging()` 写法。
- `switch_to` 只交换 `struct context` 里的 14 个寄存器（`ra`/`sp`/`s0..s11`），不做策略判断；新线程第一次运行靠 `context.ra = forkret`。

**验收标准：**

- `[Test Schedule] PASS` —— 10 分；另需输出 `this initproc, pid = 1, name = "init"`、`To U: "Hello world!!".`、`To U: "en.., Bye, Bye. :)"`。

**最终提示词：**

定稿原文如下，全文同时收录于 [`report/prompt.md`](./prompt.md) 第三节「提示词 0」。

````markdown
[PROMPT]
**任务**：修改 `kern/process/proc.c`、`kern/schedule/sched.c`、`kern/process/switch.S`，在各自的 `//LAB4 function3 YOUR CODE` 处实现内核线程的调度与上下文切换：`cpu_idle()`、`schedule()`、`proc_run()`、`switch_to()`，并把 `YOUR CODE` 替换为我的学号 `2412796`。

**操作要求**：必须在项目中修改实际文件；只填三个标记处，`sched.c` 已有的 `wakeup_proc()`、`proc.c` 中 function2 区域与既有函数一律不动；`switch.S` 只实现 `switch_to`；必须在 `-Werror` 下编译通过。

**输出要求**：使用下方信息，以工程真实定义为准。

[RELY]
```c
// kern/process/proc.h
struct context {            // 14 个字段，每个 8 字节
    uintptr_t ra, sp, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
};
struct proc_struct {
    enum proc_state state; int pid; int runs; uintptr_t kstack;
    volatile bool need_resched; struct proc_struct *parent;
    struct mm_struct *mm; struct context context; struct trapframe *tf;
    uintptr_t pgdir; uint32_t flags; ... list_entry_t list_link, hash_link;
};
extern struct proc_struct *idleproc, *initproc, *current;
extern list_entry_t proc_list;
void proc_run(struct proc_struct *proc);
void cpu_idle(void) __attribute__((noreturn));

// kern/schedule/sched.h
void schedule(void);

// 链表与状态
list_next(le) / le2proc(le, list_link) / PROC_RUNNABLE

// kern/mm/pmm.h
static inline void flush_tlb(void);        // sfence.vma

// kern/mm/pmm.c::enable_paging() —— satp 的正确写法参照
write_csr(satp, 0x8000000000000000 | (boot_pgdir_pa >> RISCV_PGSHIFT));

// kern/sync/sync.h
local_intr_save(flag) / local_intr_restore(flag)

// libs/riscv.h
SATP_MODE_SV39 = 8；RISCV_PGSHIFT = 12
lsatp() 是 Sv32 遗留，无法表达 Sv39 的 satp，不可使用

// 已提供的 forkret()（kern/process/proc.c）
static void forkret(void) { forkrets(current->tf); }
```

```
// 新线程首次运行：switch_to 末尾的 ret 跳到 context.ra = forkret，
// 之后 forkrets(tf) → __trapret → sret → kernel_thread_entry → 线程函数。
```

[GUARANTEE]
```c
void cpu_idle(void);
void schedule(void);
void proc_run(struct proc_struct *proc);
void switch_to(struct context *from, struct context *to);   // 汇编实现
```

[SPECIFICATION]
## switch_to
**Pre-Condition**：`a0 = &from->context`，`a1 = &to->context`，两者指向有效的 `struct context`。
**Post-Condition**：`from` 的 `ra/sp/s0..s11` 已写入其 context；寄存器已恢复为 `to` 的值；`ret` 返回到 `to->ra`。
**Requirements**：
- 只保存、恢复被调用者保存寄存器；调用者保存寄存器由编译器在调用点处理。
- 字段顺序与 `struct context` 一致，偏移量 = 索引 × 8（`s11` 在 104）。
- 不得触碰 `a0`/`a1` 指向之外的内存；`switch_to` 必须是全局符号。

## schedule
**Pre-Condition**：`current` 有效，`proc_list` 已初始化。
**Post-Condition**：`current->need_resched` 已清 0；已选出 `next`（`PROC_RUNNABLE`）；`next != current` 时已调用 `proc_run(next)`；`next->runs` 已加 1；全程关中断。
**Case 1**（当前不是 idleproc）：从 `current->list_link` 的下一个开始环形遍历。
**Case 2**（当前是 idleproc）：从 `&proc_list` 开始环形遍历。
**Case 3**（一圈都没有 `PROC_RUNNABLE`）：`next = idleproc`。
**Requirements**：
- `next == current` 时不得调用 `proc_run`。
- 临界区必须用 `local_intr_save/restore` 包住"清零 + 选线程 + 切换"全过程。
- 不得在临界区内调用可能睡眠或重新调度的函数。

## proc_run
**Pre-Condition**：`proc` 有效；若 `proc == current` 直接返回。
**Post-Condition**：`current == proc`；`satp` 指向 `proc->pgdir`；上下文已切换到 `proc`；返回时运行在 `proc` 的上下文中。
**Requirements**：
- 用 `write_csr(satp, (SATP_MODE_SV39 << 60) | (pgdir >> RISCV_PGSHIFT))`，禁用 `lsatp()`。
- 切页表后必须 `flush_tlb()`。
- `current` 更新、satp 切换、`switch_to` 三步在同一关中断临界区内完成。

## cpu_idle
**Post-Condition**：永不返回；`current->need_resched` 为真时调用 `schedule()`。
**Requirements**：循环体不得阻塞等待，也不得清 `need_resched`（那是 `schedule` 的职责）。
````

**实现迭代过程：**

**一、提示词的两轮调整**

*第一轮：只说了"实现调度与上下文切换四个函数"。* 问题有两处：`switch_to` 是汇编，AI 不知道 `struct context` 的真实布局，很容易按 x86 的 `push/pop` 风格写；`proc_run` 不知道怎么设置 `satp`，而 `libs/riscv.h` 里恰好有一个现成的 `lsatp()`——它是 Sv32 遗留，在 Sv39 下**不报错但不生效**，属于最危险的那类错误。

*第二轮：把体系结构事实全部写进 `[RELY]`。* 补入 `struct context` 的 14 个字段与"偏移量 = 索引 × 8"、`enable_paging()` 里 `satp` 的写法、`forkret()` 的已有实现、`local_intr_save/restore` 宏；并在 `Requirements` 中把三条硬约束写成断言式语句：「禁用 `lsatp()`」「只交换被调用者保存寄存器」「三步在同一临界区」。这一版生成结果一次编译通过。

**二、实现过程中处理的问题**

1. **`switch_to` 的偏移量必须数清楚。** `struct context` 是 14 项，`s11` 落在 104 而不是 112。若按 15 项去写，最后一条 `ld` 会读到 `to` 结构体之外，现象是切过去后某个寄存器变成垃圾值、线程跑飞。

2. **`schedule` 不能让 `next` 等于 `current`。** `proc_run()` 里虽有 `if (proc != current)` 兜底，但真正的意义是：只有自己能跑时不应再做一次毫无收益的上下文交换。因此选线程时必须显式比较。

3. **遍历起点按"当前是不是 idleproc"分开。** `idleproc` 本身也在 `proc_list` 里，如果当前是它却从"自己的下一个"起步，绕一圈很可能又选回自己，`initproc` 迟迟得不到 CPU——现象是 `proc_init()` 之后再也看不到 initproc 的输出。

4. **`flush_tlb()` 不能省。** `satp` 改了但 TLB 里仍是旧翻译，页表切换等于没做；本实验里两个内核线程用的是同一张内核页表，所以这个错误不会立刻暴露，但一旦进入用户进程阶段就会变成难以定位的随机错误。

5. **环境侧的问题：评分用的测试文件在新工具链下编译失败。** `make grade` 注入的 `tools/test_lab4.c` 里，`test_lab4_thread()` 声明为返回 `void`，却作为 `int (*)(void *)` 传给 `kernel_thread()`。新版 GCC 把这种不兼容指针类型当作错误（而不是警告），导致评分构建直接失败、三项测试全部 "no $qemu.out"。把测试函数的返回类型改成 `int` 后评分正常——**这属于测试框架与工具链的兼容问题，不在内核实现里**，因此只改测试文件、不动内核代码。

---

### 扩展练习（Challenge，思考题）

> 以下三题由部分三汇总，答案结合本实验的真实代码。

**1. `local_intr_save(intr_flag); … local_intr_restore(intr_flag);` 是如何实现开关中断的？**

`kern/sync/sync.h` 里这两个是宏，展开后分别调用 `__intr_save()` / `__intr_restore()`：

```c
static inline bool __intr_save(void) {
    if (read_csr(sstatus) & SSTATUS_SIE) {   // 当前中断是开着的
        intr_disable();                      // 清 sstatus.SIE
        return 1;
    }
    return 0;                                // 本来就关着，什么也不做
}
static inline void __intr_restore(bool flag) {
    if (flag) intr_enable();                 // 只有原本开着才重新打开
}
```

关键在于它**保存的不是整个 `sstatus`，而是"进入临界区前中断开没开"这一个事实**（一个 `bool`）。退出时只有原本开着才重新打开，于是：

- 如果进入前本来就关着（例如处在外层临界区里），退出后仍保持关闭——不会被内层"恢复"成打开；
- 因此这对宏可以**嵌套使用**，这也是它比"进入时一律关、退出时一律开"更正确的原因。

在单核、无抢占的本实验内核里，关中断就足以构成临界区；多核环境下还需要配合自旋锁（关中断只能阻止本 Hart 被中断）。

**2. `get_pte()` 中两段相似的代码，为什么这么像？结合 sv32 / sv39 / sv48 的异同解释。**

三种模式的**结构是一样的**，只是层数不同：

| 模式 | 虚拟地址宽度 | 页表级数 | 各级索引位宽 | 页表项大小 |
|---|---|---|---|---|
| Sv32 | 32 位 | 2 级 | 10 + 10（+12 位页内偏移） | 4 字节 |
| Sv39 | 39 位 | 3 级 | 9 + 9 + 9（+12） | 8 字节 |
| Sv48 | 48 位 | 4 级 | 9 × 4（+12） | 8 字节 |

三者都是"用 VPN 逐级索引页表，最后一级页表项给出物理页号与权限位"。既然每一级的动作都是**同一件事**——"取 `pdep[i]`；若无效就分配一个页表页、清零、写回 PTE；再把 `pdep` 指向下一级"——代码自然长得一模一样。Sv39 比 Sv32 多一级，所以 `get_pte()` 里出现两段几乎相同的 `if (!(*pdep & PTE_V)) { … }`；Sv48 则会再多一段。

真正的差异藏在宏里：索引位宽（`PDX1SHIFT`/`PDX0SHIFT` 是 30/21）、页表项大小（决定 `PTE_ADDR` 怎么从 PTE 里取出物理地址）、以及 `satp` 的模式编码（Sv39 是 8，Sv48 是 9）。

**3. `get_pte()` 把"查找页表项"和"分配页表页"合并在一个函数里，这种写法好吗？**

**好处**：调用者只需表达"我要这个地址对应的 pte 指针"，不必自己逐级往下走；`create` 参数又能让只读查询（如 `get_page()`、`page_remove()`）在缺页表时直接返回 `NULL`，不至于意外分配。代码量小，调用点集中。

**坏处**：职责不单一，`NULL` 同时承担了"没有建立"和"分配失败"两种含义，调用者无法区分，只能一律返回 `-E_NO_MEM`；此外在缺页处理这类"不该分配"的路径上，一旦传错 `create` 就会静默建表，掩盖真正的错误。

**是否有必要拆开**：从工程角度看值得拆——一个 `walk()` 只负责查找，一个 `alloc_level()` 只在明确需要时调用，调用者按语义显式选择，错误码也更精确。Linux 里 `pud_alloc` / `pmd_alloc` 就是分开的。但就本实验而言，页表只有三层、调用点全部可控，合并写法足够用，框架也提供了 `create` 参数作为折中，因此我们没有改动它。

---

## 五、测试与验证

**编译与运行（`make qemu`）：**

`make` 在 `-Werror` 下无警告通过。`make qemu` 的启动过程按顺序可以观察到四组关键信息：

1. **固件与内存**：OpenSBI 打出 `Domain0 Next Address : 0x0000000080200000`（说明 `-kernel` 生效、跳转地址正确），随后 `DTB Init` 报告 `Base: 0x80000000 Size: 128 MB`。
2. **物理内存与页表**：`memory management: default_pmm_manager` → `check_alloc_page() succeeded!` → `check_pgdir() succeeded!` → `check_boot_pgdir() succeeded!`，说明部分一的映射接口与 lab2 补回都正确。
3. **虚拟内存与进程**：`use SLOB allocator`、`kmalloc_init() succeeded!`、`check_vma_struct() succeeded!`、`check_vmm() succeeded.`，随后 `++ setup timer interrupts`。
4. **线程被真正调度执行**：

```
this initproc, pid = 1, name = "init"
To U: "Hello world!!".
To U: "en.., Bye, Bye. :)"
```

这三行来自 `init_main()`，能出现就说明 `proc_run` / `switch_to` 已经完成了一次真实的上下文切换——`idleproc` 让出 CPU、`initproc` 第一次运行。

> **关于末尾的 `kernel panic ... process exit!!.`**：`init_main()` 返回后，`kernel_thread_entry` 会调用 `do_exit()`，而本实验的 `do_exit()` 仍是框架提供的 `panic("process exit!!.")` 占位实现。因此这条 panic 是**预期行为**，不属于本实验的错误；进程退出与回收是后续实验的内容。

![make qemu 运行结果](./images/lab4_qemu.png)

**自动评分（`make grade`）：**

| 测试 | 检查内容 | 分值 | 结果 |
|---|---|---|---|
| Test PMM | 页表映射建立/撤销与引用计数 | 10 | PASS |
| Test Process Create | `idleproc` / `initproc` 的 PCB 字段 | 10 | PASS |
| Test Schedule | 线程创建、轮转调度、切换执行 | 10 | PASS |
| **总分** | | **30** | **30/30** |

![make grade 评分结果](./images/lab4_grade.png)

---

## 六、实验总结与收获

### 对操作系统的理解

**1. 本实验中重要的知识点，及与 OS 原理中对应知识点的关系**

| 本实验中的知识点 | 对应 OS 原理中的知识点 | 含义、关系与差异 |
|---|---|---|
| `page_insert` / `page_remove` 与引用计数 | 虚拟内存映射的建立与撤销、页表管理 | 原理讲"虚拟地址到物理地址的映射由页表维护"；本实验把它落到"建立或撤销一条 PTE、同步引用计数、失效 TLB"三件事上。**关系**：完全对应。 |
| 三级页表与 `get_pte()` 逐级索引 | 多级页表、地址翻译 | 原理讲多级页表是为了节省页表空间；本实验的 `get_pte()` 就是逐级索引并"缺页表页则分配"的过程。**差异**：本实验预映射、没有缺页异常，页表页是主动分配的。 |
| `struct proc_struct`（PCB） | 进程控制块 | 原理中 PCB 记录进程全部状态；本实验的 PCB 只含调度所需字段（状态、内核栈、上下文、页表、父子关系），没有文件描述符、信号等资源。**关系**：是 PCB 的一个最小子集。 |
| `struct context` 只保存 14 个寄存器 | 上下文切换 | 原理讲切换要保存/恢复寄存器现场；本实验利用"切换发生在函数内部"这一点，只保存**被调用者保存**寄存器。**差异**：真正的用户进程切换还要换页表、换栈，本实验两个内核线程共用一张页表。 |
| `trapframe` 与 `switch_to` 是两套现场 | 中断上下文 vs 进程上下文 | 原理上两者常被混为一谈；本实验区分得很清楚：`trapframe` 保存进入 Trap 那一刻的**全部**寄存器（36 slot），用于"回到被打断的点"；`context` 只保存切换所需的 14 个，用于"换一个线程继续跑"。 |
| `kernel_thread → do_fork → copy_thread` | 进程创建（fork） | 原理讲 `fork` 复制父进程；本实验的 `do_fork` 就是简化版：复制 PCB、分配内核栈、复制并改写 trapframe（`a0 = 0` 表示子进程）。**差异**：没有真正的地址空间复制（`copy_mm` 占位），子进程是内核线程。 |
| 内核线程与用户进程 | 线程与进程的区别 | 原理讲线程共享地址空间、进程独立；本实验的所有线程共享内核页表，`mm` 为 `NULL`，正是"轻量级进程"。 |
| `schedule` 的轮转与 `need_resched` | 调度策略与调度点 | 原理讲时间片轮转、调度时机；本实验是 FIFO + 轮转选择，调度点只有 `cpu_idle` 与测试里显式置位 `need_resched` 两处。**差异**：没有时钟驱动的抢占式调度。 |
| `local_intr_save/restore` 与临界区 | 原子操作、临界区保护 | 原理讲临界区要靠锁或关中断；本实验单核无抢占，关中断即可，且这对宏保存的是"原来开没开"，可嵌套。**差异**：多核场景下关中断不够，还需自旋锁。 |

**2. OS 原理中很重要、但本实验没有对应上的知识点**

1. **用户态进程与系统调用**：本实验所有线程都运行在 S 模式，`mm` 全为 `NULL`，没有 U 模式、没有 `ecall` 系统调用入口，也没有用户栈与用户页表的切换。这是 lab5 的内容。
2. **按需分页与缺页异常**：地址空间是建立时一次性预映射的，`do_pgfault()` 在本实验中不会被触发，更没有写时复制。lab9 才会真正处理页错误。
3. **页面置换**：内存不够时需要换出到磁盘，本实验的物理内存固定 128 MB、只分配不回收，没有置换算法与 swap。
4. **阻塞与唤醒的真实使用**：`wakeup_proc()` 在本实验里只在 `do_fork` 里把新线程置成就绪，并不存在"等待某事件而睡眠"的路径，也没有等待队列。
5. **优先级与更复杂的调度算法**：只有 FIFO/轮转，没有优先级、多级反馈队列、时间片计算；调度点也不由时钟中断驱动。
6. **多核（SMP）与 IPI**：本实验假定单 Hart，`current` 只是普通全局变量，关中断就能保护；多核还要处理每核的 `current`、TLB shootdown、核间中断。
7. **进程间通信与资源管理**：没有管道、共享内存、信号，也没有文件描述符表；`do_exit()` 还是 `panic` 占位，没有真正的进程回收与僵尸态处理。
8. **内存映射文件与写时复制**：`mmap` 一类的机制完全没有涉及，`vma_struct` 目前只被测试用到。

### AI 协作开发的经验

**1. 汇编任务：把结构体布局原样贴进提示词，比描述它有效得多。**

`switch_to` 是纯汇编，正确性完全取决于 `struct context` 的字段顺序与偏移量。第一版提示词只说了"保存和恢复上下文"，AI 很可能按 x86 的习惯去写 `push/pop`，或者把 14 项当成 15 项。后来在 `[RELY]` 里直接写出字段列表并注明"偏移量 = 索引 × 8，`s11` 在 104"，生成结果就没有再错过。

**2. 体系结构陷阱要用"禁止"来写，而不是"建议"。**

`libs/riscv.h` 里有一个现成的 `lsatp()`，看起来正好是"设置 satp"的函数——但它来自 Sv32，模式位写死了 Sv32，在 Sv39 下**不报错、不生效**。这类"能编译、能跑、但语义错"的坑最难查。所以在提示词的 `Requirements` 里直接写"禁用 `lsatp()`，改用 `write_csr`"，并在 `[RELY]` 里给出 `enable_paging()` 的正确写法作为参照。

**3. "能编译"和"能评分"是两件事。**

三部分合并后 `make` 一次通过，但第一次 `make grade` 是 0/30——原因不在内核代码，而是评分脚本注入的 `test_lab4.c` 在新版 GCC 下编译失败（`void (*)(void *)` 传给 `int (*)(void *)`，GCC 14 起这类不兼容指针转换从警告变成了错误）。这提醒我们：**验证要走完整的评分链路**，单看 `make` 成功并不能说明测试会通过。

**4. 收口阶段的体会：边界问题是最后才会浮现的那一类。**

三个人并行开发，各自的部分单独看都没问题：部分一的映射接口正确、部分二的 PCB 与首次运行链路正确、部分三的切换正确。但真正跑起来之后，任何一处对不上都会表现为"调度过去了但立刻崩"，很难单看谁的代码判断。有效的做法是**沿着一条链逐段验证**：先确认 `check_pgdir() succeeded!`（映射可用），再确认 `idleproc`/`initproc` 的字段（对象正确），最后确认 initproc 的三行输出（切换正确）——输出能一路打出来，就说明整条链打通了。

---

## 七、提交前自检

- [x] 四处 lab2/lab3 代码已补回，五处 `LAB4 functionN YOUR CODE` 已填并替换为学号
- [x] `SAVE_ALL`、`struct trapframe`、`trap()` 签名未被修改；未改动框架提供的测试函数
- [x] `make` 在 `-Werror` 下无警告通过，`make grade` 30/30
- [x] 全文无"待填写"；每个功能模块都有「最终提示词」与「实现迭代过程」
- [x] `prompt.md` 三节齐全，迭代记录表已填
- [ ] `images/` 下截图齐全且为真实截图，路径为相对路径
- [ ] 成果在 `lab4` 分支，根目录只有 `code/` 与 `report/`
