# Lab 4 任务分工说明

> 本文件是本小组的内部工作文档，用于明确 lab4 三个部分的负责人、具体工作内容与交付要求。
> 仓库：<https://github.com/YSY-yt/os-lab-2026>　分支：`lab4`
> 实验指导书：<http://oslab.mobisys.top/lab2026/_book/lab4/lab4.html>
> **✅ 本实验已完成**：`make grade` 得分 **30/30**（PMM / Process Create / Schedule 三项全部通过）。
> 终版提交：`lab4` 分支 `ecb4f11`。下文的"当前状态""编译不过"等表述均指**开工时的初始状态**，留作记录。

---

## 零、先读这一节

### 0.1 分工总览

| | 部分一 | 部分二 | 部分三 |
|---|---|---|---|
| **主题** | 虚拟内存页操作 | 进程创建及初始化 | 进程调度 |
| **覆盖的练习** | 功能模块一 | 功能模块二 | 功能模块三 |
| **要写的代码** | `kern/mm/pmm.c` 的 `page_insert()` / `page_remove()` | `kern/process/proc.c` 的 `alloc_proc` / `proc_init` / `kernel_thread` / `do_fork` / `copy_thread` / `copy_mm` | `cpu_idle` / `proc_run`（`proc.c`）、`schedule()`（`sched.c`）、`switch_to`（`switch.S`） |
| **还要补回** | lab2 的 `page_init()`、lab3 的 `RESTORE_ALL` / `__alltraps` | lab2 的 Best-Fit、lab3 的两个 handler | 无 |
| **对应评分** | `[Test PMM]` **10 分** | `[Test Process Create]` **10 分** | `[Test Schedule]` **10 分** |
| **代码量** | 小（2 个函数） | 大（6 个函数，本实验核心） | 中（4 个函数，含一段汇编） |
| **负责人** | 2413628-黄乐仁 | 2410868-孙锘梵 | 2412796-杨斯源（组长） |

**难度分布说明**：功能模块二最重、功能模块一最轻，所以把"补回 lab2/lab3 代码"的机械性工作相应地分给部分一和部分二（谁在 lab2/lab3 写过谁来补），部分三另外承担报告收口与全量验证。

**本次实验没有必须实现的 Challenge**，只有三道思考题（见指导书最后一节），由部分三汇总进报告。

### 0.2 提交规范（老师要求，不可违反）

- 一次实验一个分支：lab4 的成果**全部提交到 `lab4` 分支**，不要提交到 `main`。
- `lab4` 分支根目录下必须是两个文件夹：`code/` 与 `report/`。
- `report/` 下必须包含 `report.md`、`prompt.md`、`images/`。
- 报告图片一律用相对路径，例如 `./images/lab4_grade.png`。
- **保留 `labN` 与 `YOUR CODE` 注释，并把 `YOUR CODE` 替换为自己的学号。**

### 0.3 环境：已经替你们适配好了

仓库里的 `lab4` 分支代码**已经改好四处**，直接 `make` 即可，不需要重复踩坑：

| 已做的改动 | 原因 |
|---|---|
| `Makefile`、`tools/grade.sh`：`-device loader,file=...,addr=0x80200000` → **`-kernel bin/ucore.img`** | QEMU ≥ 8 自带 OpenSBI 是 `fw_dynamic` 变体，跳转地址必须由 QEMU 经 `-kernel` 告知；否则 `Domain0 Next Address` 为 `0x0`，**内核永不执行、三项测试全 0** |
| `kern/init/entry.S`：`boot_page_table_sv39` 补回**低 1G 恒等映射** | 打开分页那一瞬间 PC 还在物理地址 `0x8020xxxx`，这一页没映射会立即取指页错误（lab2/lab3 都踩过） |
| `kern/mm/default_pmm.c`：删除 `#include <best_fit_pmm.h>` | 工程里没有这个头文件（lab4 只有 `default_pmm.h`），保留则编译直接失败 |
| `kern/mm/kmalloc.c`：`kva2page(kva)` → `kva2page((void *)kva)` | 骨架把 `unsigned long` 直接传给指针参数，新版 GCC 的 `-Wint-conversion` 直接报错 |
| `Makefile`：GDB 命令自动探测 `riscv64-unknown-elf-gdb` / `gdb-multiarch` | apt 仓库里没有前者 |

另外 `tools/grade.sh` 的 GDB 断点已改为"跑满固定时间再收日志"——原脚本按老平台约定推导断点地址，在 riscv64 上不可用。

**当前编译现状（已在 WSL 里实测）**：`make -k` 只剩三处错误，全部落在"待补回"的标记位置上，其余文件（含 `entry.S`、`switch.S`、`vmm.c`）都能编译通过：

```
kern/mm/default_pmm.c: 'free_list' / 'nr_free' undeclared   → 部分二补回 Best-Fit
kern/trap/trap.c: implicit declaration of interrupt_handler/exception_handler → 部分二补回 lab3
kern/mm/pmm.c: implicit declaration of page_init            → 部分一补回 lab2
```

> 注意新版 GCC 把 `-Wimplicit-function-declaration` 当作**错误**，所以函数必须先定义再调用——各处标记位置都在调用点之前，按标记填就行。

**三条避坑提醒：**

1. **必须在交互式 Ubuntu 终端里跑 `make`**（非交互 shell 拿不到工具链 `PATH`）。
2. **退出 QEMU**：按住 `Ctrl` 再按 `a`，都松开后单独按 `x`。
3. **分支刚建好时编译不过**——四处 lab2/lab3 代码未补回、五处 `LAB4 functionN YOUR CODE` 未实现。**这是预期的**，正好是任务本身。（现已全部补齐，`make` 零警告通过、`make grade` 30/30。）

### 0.4 自动评分是怎么打的

`make grade` 会把整棵树复制到 `/tmp/lab4_grade`，用 `tools/inject.py` 注入测试，然后编译运行一次，在串口日志里匹配固定字符串：

| 测试 | 注入位置 | 分值 | 通过的标志 |
|---|---|---|---|
| Test PMM | `pmm_init()` 末尾 | **10** | `[Test PMM] PASS` |
| Test Process Create | `proc_init()` 末尾 | **10** | `[Test Process Create] PASS` |
| Test Schedule | `init_main()` 开头 | **10** | `[Test Schedule] PASS`，另需 `this initproc, pid = 1, name = "init"`、`To U: "Hello world!!".`、`To U: "en.., Bye, Bye. :)"` |

**总分 30 分。**

测试代码（`code/tools/test_lab4.c`）会检查这些细节，值得提前知道：

- **Test PMM**：`page_insert(boot_pgdir_va, p1, 0, 0)` 后 `page_ref(p1) == 1`；把 `p1` 重新映射到 `PGSIZE` 覆盖 `p2` 时，要求 `page_ref(p1) == 2` 且 `page_ref(p2) == 0`；对同一物理页重复 `page_insert`（只改权限）时**引用计数不得增加、空闲页数不得变化**；`page_remove` 连续调用两次不得出错。
- **Test Process Create**：`idleproc` 的 `pid == 0`、`state == PROC_RUNNABLE`、`kstack == (uintptr_t)bootstack`、`parent == NULL`、`mm == NULL`、`tf == NULL`、`runs == 0`、`flags == 0`、`context.ra == 0`；`initproc` 的 `pid == 1`、`parent == idleproc`、`context.ra != 0`、`context.sp != 0`。
- **Test Schedule**：用 `kernel_thread()` 现造两个线程，每个线程里三次 `need_resched = 1; schedule();`，主线程再调度 8 次，最后断言两个线程都真正跑过。**这一项同时检验了部分二的 `kernel_thread` 和部分三的 `schedule`/`switch_to`。**

> 测试是**直接调用**这些函数的，所以实现里不能依赖"只在某种启动阶段才成立"的假设，更不能 `panic()`。

### 0.5 ⚠️ 三部分互相耦合

**（1）代码树在补回全部 lab2/lab3 代码前编译不过**

- `pmm.c` 调用 `page_init()`（部分一补）→ 未补则链接失败；
- `default_pmm.c` 缺管理函数（部分二补）→ 编译失败；
- `trap.c` 缺两个 handler（部分二补）→ 编译失败；
- `trapentry.S` 缺 `RESTORE_ALL`（部分一补）→ 汇编失败；
- `proc_init()` 调用 `kernel_thread`（部分二写）、`cpu_idle()`（部分三写）→ 未写则编译失败。

**结论：任何一方单独改完都跑不起来**，必须先补回遗留代码，再各自实现功能。

**（2）谁改哪些文件**

| 部分 | 改动的文件 |
|---|---|
| 部分一 | `kern/mm/pmm.c`、`kern/trap/trapentry.S` |
| 部分二 | `kern/mm/default_pmm.c`、`kern/trap/trap.c`、`kern/process/proc.c`（function2 区域） |
| 部分三 | `kern/process/proc.c`（function3 区域）、`kern/schedule/sched.c`、`kern/process/switch.S` |

部分二与部分三**都会改 `proc.c`**，但落在两个不同的标记区（`//LAB4 function2` 与 `//LAB4 function3`），冲突面很小。**提交前务必先 `git pull`。**

**建议的协作方式：**

1. 先各自补回自己负责的 lab2/lab3 代码（机械性工作，但能让树尽快可编译）；
2. 再各自实现功能模块；
3. **最后一起做集成验证**：`git pull` → `make grade` → 看到 **30/30** 才算完。

### 0.6 关于"补回 lab2/lab3 代码"

指导书原文：

> **本实验依赖实验2/3。请使用你此前设计的提示词生成相应实验代码，并将生成结果填写到本实验代码中标有"lab2"/"lab3"注释的对应位置，确保代码能够正确编译。**

代码里共四处：

| 位置 | 补什么 | 谁最合适 |
|---|---|---|
| `kern/mm/default_pmm.c`（第 6 行附近） | lab2 的 Best-Fit 管理模块 | 孙锘梵（lab2 写过） |
| `kern/mm/pmm.c`（第 90 行附近） | lab2 的 `page_init()` | 黄乐仁（lab2 写过） |
| `kern/trap/trap.c`（第 86 行附近） | lab3 的 `interrupt_handler()` / `exception_handler()` | 孙锘梵（lab3 写过） |
| `kern/trap/trapentry.S`（第 56、63 行附近） | lab3 的 `RESTORE_ALL` 与 `__alltraps` 调用 | 黄乐仁（lab3 写过） |

> ⚠️ **两个容易翻车的点**：
> 1. 本实验的管理器结构体叫 **`default_pmm_manager`**（`default_pmm.h` 里已 `extern` 声明），但内部函数仍叫 `best_fit_*`、测试函数仍叫 `best_fit_check()`。从 lab2 分支拷代码时**只改结构体名**，逻辑别动。
> 2. `basic_check()` 与 `best_fit_check()` **由框架提供，不得修改或重复实现**。lab2 的 `best_fit_pmm.c` 自带同名函数，整文件覆盖会导致重复定义。
>
> 参考：lab2 / lab3 分支的 `code/` 与 `report/prompt.md` 里有完整实现和当时的提示词。

---

## 一、部分一：虚拟内存页操作

**负责人：2413628-黄乐仁**

### 1.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| 补回 `page_init()` | `code/kern/mm/pmm.c` 的 `//lab2 ...` 处 | 编译通过 |
| 补回 `RESTORE_ALL` / `__alltraps` | `code/kern/trap/trapentry.S` 的两处 `#lab3:` 处 | 汇编通过 |
| `page_insert()` | `code/kern/mm/pmm.c` 的 `//LAB4 function1 YOUR CODE` | 通过 Test PMM |
| `page_remove()` | 同上（可另加静态辅助函数 `page_remove_pte()`） | 同上 |
| 报告「前置」与「功能模块一」 | `report/report.md` | 含最终提示词、迭代过程 |
| 3 份提示词 | `report/prompt.md` 第一节 | 四段式 |

### 1.2 实现要点

**`page_insert(pgdir, page, la, perm)`**

```c
pte_t *ptep = get_pte(pgdir, la, 1);        // 1 = 必要时分配中间级页表页
if (ptep == NULL) return -E_NO_MEM;         // 分配失败：不能留下半完成映射
page_ref_inc(page);                         // 先加引用，覆盖旧项时会回退
if (*ptep & PTE_V) {                        // 原地址已有映射
    struct Page *old = pte2page(*ptep);
    if (old == page) page_ref_dec(page);    // 同一物理页：净增 0，只改权限
    else             page_remove_pte(pgdir, la, ptep);   // 不同物理页：撤销旧映射
}
*ptep = pte_create(page2ppn(page), PTE_V | perm);
tlb_invalidate(pgdir, la);
return 0;
```

**`page_remove(pgdir, la)`**

```c
pte_t *ptep = get_pte(pgdir, la, 0);        // 0 = 不分配
if (ptep == NULL) return;                   // 都没建立过页表：安静返回
if (*ptep & PTE_V) page_remove_pte(pgdir, la, ptep);
else               tlb_invalidate(pgdir, la);
```

**`page_remove_pte`（建议写成静态辅助函数）**

```c
if (*ptep & PTE_V) {
    struct Page *page = pte2page(*ptep);
    if (page_ref_dec(page) == 0) free_page(page);   // 最后一个引用才真正释放
    *ptep = 0;                                      // 必须清空页表项
    tlb_invalidate(pgdir, la);
}
```

**三个必须同步的东西**：页表项内容、物理页引用计数、TLB。少任何一个，测试里都会出现"看起来改了、实际没生效"。

### 1.3 验收标准

- ✅ `make grade` 中 `[Test PMM] PASS`（10 分）
- `make qemu` 里能看到 `check_pgdir() succeeded!`

### 1.4 完成标准（自检）

- [x] `[Test PMM]` 通过
- [x] 同一物理页重复 `page_insert` 时引用计数不增加、`nr_free_pages()` 不变化
- [x] `page_remove` 对未映射地址可重复调用
- [x] `//LAB4 function1` 注释保留，`YOUR CODE` 换成学号
- [x] `prompt.md` 第一节三份提示词已填全
- [x] 报告里写了 bug 与解决过程

### 1.5 易错点

- ❌ 直接 `*ptep = ...` 覆盖而**不处理旧页的引用计数** → `page_ref(p2)` 应为 0 却仍是 1。
- ❌ 同一物理页重复映射时也 `page_ref_inc` → 引用计数多 1，后续 `page_remove` 释放不掉。
- ❌ 忘记 `tlb_invalidate` → 页表改了但 MMU 仍用旧翻译。
- ❌ `get_pte(..., 1)` 返回 `NULL` 时没返回 `-E_NO_MEM`。
- ❌ `page_remove` 假定一定存在映射，对未映射地址解引用空指针。

---

## 二、部分二：进程创建及初始化

**负责人：2410868-孙锘梵**

### 2.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| 补回 Best-Fit | `code/kern/mm/default_pmm.c` 的 `//lab2 ...` 处 | 结构体名改为 `default_pmm_manager` |
| 补回两个 handler | `code/kern/trap/trap.c` 的 `//lab3 ...` 处 | 编译通过 |
| 六个函数 | `code/kern/process/proc.c` 的 `//LAB4 function2 YOUR CODE` | 通过 Test Process Create |
| 报告「功能模块二」 | `report/report.md` | 含最终提示词、迭代过程 |
| 4 份提示词 | `report/prompt.md` 第二节 | 四段式 |

### 2.2 实现要点

**首次运行链路（本模块的核心）**

```
kernel_thread()  ──► do_fork() ──► copy_thread()
     │                                 │
     │ 构造临时 trapframe：             │ 把 tf 复制到新线程内核栈顶
     │   s0 = fn, s1 = arg             │ context.ra  = forkret
     │   epc = kernel_thread_entry     │ context.sp  = 新内核栈上的 tf
     ▼                                 ▼
schedule() ──► proc_run() ──► switch_to()   // 切到新线程
                                   │  ret 到 context.ra = forkret
                                   ▼
                              forkret → forkrets(tf)   // sp = tf
                                   ▼
                              __trapret → RESTORE_ALL → sret → pc = epc
                                   ▼
                              kernel_thread_entry:  a0 = s1; jalr s0
                                   ▼
                              线程函数 fn(arg) 开始执行
```

这条链路任何一环对不上（`context.ra`、`context.sp`、`tf->epc`、`s0/s1`），都会表现为"调度过去了但立刻崩"。

**`alloc_proc()`**：`kmalloc` 一个 `proc_struct`，**全部字段清零**后再设安全初值——`state = PROC_UNINIT`、`pid = -1`、`pgdir = boot_pgdir_pa`（物理地址）、`need_resched = 0`、`parent = mm = tf = NULL`、`name[0] = '\0'`、`list_link`/`hash_link` 置空。

**`proc_init()`**：初始化 `proc_list` 与 `hash_list` → `alloc_proc()` 得到 `idleproc` → 把 `idleproc->pid = 0`、`state = PROC_RUNNABLE`、`kstack = (uintptr_t)bootstack`、`need_resched = 1`、`current = idleproc`、`nr_process = 1` → `kernel_thread(init_main, "Hello world!!", 0)` 得到 `initproc`（pid 应为 1）。

> `idleproc` **不是 fork 出来的**，它直接复用 `kern_init` 以来的执行现场，所以 `kstack` 指向启动栈、`tf` 为 `NULL`、`context.ra` 为 0——测试正是这样断言的。

**`kernel_thread(fn, arg, clone_flags)`**：栈上开一个 `struct trapframe tf`，清零；`tf.gpr.s0 = fn`、`tf.gpr.s1 = arg`；`tf.epc = kernel_thread_entry`；`tf.status` 取 `read_csr(sstatus)` 后**置 SPP、置 SPIE、清 SIE**（首次"返回"到 S 模式且开中断）；`return do_fork(clone_flags, ..., &tf)`。

**`do_fork()`**：`alloc_proc` → `setup_kstack` → `copy_mm` → `copy_thread` → `get_pid` → `hash_proc` → `list_add(&proc_list, ...)` → `nr_process++` → `wakeup_proc` → 返回 pid。**前三步失败要逐级回滚**（`kfree` / `put_kstack`）。

**`copy_thread(proc, esp, tf)`**：`proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;`（放在内核栈顶）→ `*(proc->tf) = *tf` → `proc->tf->gpr.sp = esp`（新线程首次运行的 sp，必须是可用地址）→ `proc->tf->gpr.a0 = 0`（子线程返回 0）→ `proc->context.ra = (uintptr_t)forkret` → `proc->context.sp = (uintptr_t)proc->tf`。

**`copy_mm()`**：本阶段内核线程 `current->mm == NULL`，直接返回 0 即可，保持接口形态供后续实验扩展。

### 2.3 验收标准

- ✅ `make grade` 中 `[Test Process Create] PASS`（10 分）
- `make qemu` 中能看到 `this initproc, pid = 1, name = "init"` 等输出（这部分还依赖部分三的调度）

### 2.4 完成标准（自检）

- [x] `[Test Process Create]` 通过
- [x] `idleproc` 各字段与测试断言一致
- [x] `initproc->context.ra != 0`、`context.sp != 0`
- [x] Best-Fit 补回后 `basic_check()`/`best_fit_check()` 未被改动或重复定义
- [x] 未修改 `trap.c` 中规定以外的函数
- [x] 注释保留、`YOUR CODE` 换成学号
- [x] `prompt.md` 第二节四份提示词已填全

### 2.5 易错点

- ❌ `alloc_proc` 只设了部分字段，其余是 `kmalloc` 回来的垃圾 → 测试断言 `tf == NULL`、`flags == 0` 直接失败。
- ❌ `idleproc` 用 `kernel_thread` 创建（它必须复用当前现场）。
- ❌ `copy_thread` 里 `context.ra` 写成 `kernel_thread_entry` 而不是 `forkret` → 首次调度时 `ret` 跳错地方。
- ❌ `tf->gpr.sp` 设成 0 或栈内不可用位置 → 线程函数一压栈就崩。
- ❌ `do_fork` 失败路径不释放已分配的内核栈/PCB。
- ❌ 忘了 `wakeup_proc`，新线程一直是 `PROC_UNINIT`。

---

## 三、部分三：进程调度

**负责人：2412796-杨斯源（组长）**

### 3.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| `cpu_idle()` | `code/kern/process/proc.c` 的 `//LAB4 function3` | 空闲循环，必要时让出 CPU |
| `proc_run()` | 同上 | 更新 current、切页表、`switch_to` |
| `schedule()` | `code/kern/schedule/sched.c` | 选线程 + 关中断临界区 |
| `switch_to()` | `code/kern/process/switch.S` | 保存/恢复 `struct context` 的 14 个寄存器 |
| 报告 3.1 / 3.2 / 五 / 六节、`prompt.md` 汇总 | `report/` | 收口 |
| 全量验证 | `make grade` 30/30 + 截图 | 收口 |

### 3.2 实现要点

**`schedule()`**

```c
bool intr_flag;
local_intr_save(intr_flag);
{
    current->need_resched = 0;
    // 起点：当前是 idleproc → 从表头；否则 → 从自己下一个（轮转）
    // 找不到就绪线程 → 退回 idleproc
    if (next != current) proc_run(next);
}
local_intr_restore(intr_flag);
```

**`proc_run(proc)`**

```c
if (proc == current) return;
bool intr_flag;
local_intr_save(intr_flag);
{
    struct proc_struct *prev = current;
    current = proc;
    write_csr(satp, 0x8000000000000000 | (proc->pgdir >> RISCV_PGSHIFT));
    flush_tlb();
    switch_to(&(prev->context), &(proc->context));
}
local_intr_restore(intr_flag);
```

> **`lsatp()` 不可用**：`libs/riscv.h` 里的 `lsatp()` 是 Sv32 遗留，表达不了 Sv39 的 satp。照 `kern/mm/pmm.c::enable_paging()` 的写法直接 `write_csr(satp, ...)`。

**`switch_to(from, to)`**（`struct context` 14 个字段 × 8 字节 = 112 字节）

```asm
.globl switch_to
switch_to:
    sd ra,   0(a0)
    sd sp,   8(a0)
    sd s0,  16(a0)
    ...            # s1..s10 依次 24..96
    sd s11,104(a0)
    ld ra,   0(a1)
    ld sp,   8(a1)
    ...
    ld s11,104(a1)
    ret            # 返回到 to->ra（第一次是 forkret）
```

**`cpu_idle()`**

```c
while (1) {
    if (current->need_resched) schedule();
}
```

### 3.3 验收标准

- ✅ `make grade` 中 `[Test Schedule] PASS`（10 分）
- 同时需要 initproc 的正常输出（`this initproc, pid = 1, ...`、`To U: "Hello world!!".`、`To U: "en.., Bye, Bye. :)"`）

### 3.4 完成标准（自检）

- [x] `[Test Schedule]` 通过，`make grade` 30/30
- [x] `schedule`/`proc_run` 的临界区确实关中断
- [x] 页表切换用 `write_csr(satp, ...)`，未使用 `lsatp()`
- [x] `switch_to` 只做上下文交换，不掺调度逻辑
- [x] 注释保留、`YOUR CODE` 换成学号
- [x] `prompt.md` 第三节提示词已填全，迭代记录表已填

### 3.5 易错点

- ❌ 忘了关中断 → `current`、satp 与实际执行现场错位（现象是随机崩、串日志）。
- ❌ `switch_to` 偏移量算错（`struct context` 是 `ra, sp, s0..s11` 共 14 项，最后一项 `s11` 在 104）。
- ❌ 调度时把**当前线程本身**选成 next 又调用 `proc_run` → 自我切换，栈被自己覆盖。
- ❌ 找不到就绪线程时把 `next` 留成 `NULL` → 空指针崩溃（应退回 `idleproc`）。
- ❌ 用 `lsatp()` 切页表 → Sv39 下无效，切完还在旧地址空间。

---

## 四、收口清单（提交前逐条打勾）

### 代码

- [x] `cd code && make clean && make` 编译通过且无警告（本工程只用 `-Wall`，没有 `-Werror`）
- [x] `make qemu` 能启动到 `initproc` 输出
- [x] `make grade` 得到 **30/30**
- [x] 四处 lab2/lab3 代码已补回、五处 `LAB4 functionN YOUR CODE` 已填并替换为学号
- [x] `SAVE_ALL`、`struct trapframe`、`trap()` 签名未被修改
- [x] `basic_check()` / `best_fit_check()` 未被修改或重复实现
- [x] `bin/`、`obj/` 未被提交

### 报告

- [x] 六节齐全，**全文无"待填写"**
- [x] 每个模块都有「最终提示词」**和**「实现迭代过程」，无空标题
- [x] 报告要求第 4 条（每个练习的 spec/prompt 怎么写）已覆盖
- [x] 报告要求第 5 条（AI 生成代码的 bug 与解决）已覆盖
- [x] 知识点对应表、未覆盖清单已写
- [x] 三道 Challenge 思考题已答
- [x] 三处「负责人」已署名，分工表与完成日期已填

### 提示词与图片

- [x] `prompt.md` 三节齐全，迭代记录表已填
- [x] `report/images/` 下有 `lab4_qemu.png` 与 `lab4_grade.png`
- [x] 报告中的图片链接在 GitHub 上能正常显示

### 分支

- [x] 成果在 `lab4` 分支，根下只有 `code/` 与 `report/`
- [x] 已 `git push`

---

## 五、命令速查

```bash
cd ~/os-lab-2026/code          # 组长机器上是 /mnt/d/os-lab-2026/code

make                    # 编译
make qemu               # 运行（退出：Ctrl+A 松开再按 x）
make grade              # 自动评分（30 分）
make clean

make debug / make gdb   # 调试（两个终端）
pkill -f qemu-system-riscv64

git pull && git add -A && git commit -m "lab4: ..." && git push
```

---

## 六、参考：lab4 官方要求原文

**实验目的**（`lab4_1_goals.html`）：

> - 了解虚拟内存管理的基本结构，掌握虚拟内存的组织与管理方式
> - 了解内核线程创建/执行的管理过程
> - 了解内核线程的切换和基本调度过程

**练习**（`lab4_2_1_exercises.html`）——三个功能模块，**均要求设计提示词借助 AI 完成**：

> **功能模块一：虚拟内存页操作功能**（`kern/mm/pmm.c`）：`page_insert()`、`page_remove()`
>
> **功能模块二：进程创建及初始化功能**（`kern/process/proc.c`）：`alloc_proc()`、`proc_init()`、`kernel_thread()`、`do_fork()`、`copy_thread()`、`copy_mm()`
>
> **功能模块三：进程调度**：`cpu_idle()`（`proc.c`）、`schedule()`（`sched.c`）、`proc_run()`（`proc.c`）、`switch_to()`（`switch.s`）

**扩展练习 Challenge（思考题）**：

> - 说明语句 `local_intr_save(intr_flag); ... local_intr_restore(intr_flag);` 是如何实现开关中断的。
> - `get_pte()` 中有两段形式类似的代码，结合 sv32、sv39、sv48 的异同，解释这两段代码为什么如此相像。
> - `get_pte()` 把页表项查找和页表页分配合并在一个函数里，你认为这种写法好吗？有没有必要拆开？

**报告要求**（`lab4_5_labs_requirement.html`）：

> 1. 基于 markdown 格式完成实验报告，以文本说明为主。
> 2. 说明自己对本章节整体逻辑线的理解，即本章节是按照什么顺序、围绕哪些功能逐步实现的。
> 3. 说明自己对每个功能的核心函数或功能模块的理解。
> 4. 说明每个练习的 spec 是怎么写的，prompt 是怎么写的。
> 5. 说明在使用提示词让 AI 生成代码的过程中出现了什么 bug，以及你是怎么解决的。
> 6. 列出你认为本实验中重要的知识点，以及与对应的 OS 原理中的知识点，并简要说明二者的含义、关系和差异。
> 7. 列出你认为 OS 原理中很重要、但在本实验中没有对应上的知识点。
>
> 注意有"lab4"的注释，代码中所有需要完成的地方（challenge 除外）都有"lab4"和"YOUR CODE"的注释，请在提交时特别注意保持注释，并将"YOUR CODE"替换为自己的学号。

---

## 七、完成记录

| 部分 | 负责人 | 完成内容 | 提交 |
|---|---|---|---|
| 部分一 | 2413628-黄乐仁 | 补回 `page_init()`、`RESTORE_ALL`/`__alltraps`；实现 `page_insert()`/`page_remove()` | `df42d22`（代码）、`af5a7e9`（报告） |
| 部分二 | 2410868-孙锘梵 | 补回 Best-Fit、两个 handler；实现 `alloc_proc`/`proc_init`/`kernel_thread`/`do_fork`/`copy_thread`/`copy_mm` | `508a019`（代码）、`e79ea02`（报告） |
| 部分三 | 2412796-杨斯源 | 实现 `schedule`/`proc_run`/`switch_to`/`cpu_idle`；报告 3.1/3.2/五/六节、Challenge 与 `prompt.md` 汇总；全量验证 | `c2d538b`（代码与报告）、`ecb4f11`（截图与收口） |

**最终验证**

- `make` 编译通过、无警告（本工程只用 `-Wall`，没有 `-Werror`）
- `make qemu`：`check_pgdir() succeeded!` → `check_vmm() succeeded.` → initproc 三行输出；末尾 `do_exit` 的 panic 为框架占位，属预期
- `make grade`：**30/30**（PMM 10 + Process Create 10 + Schedule 10）

**过程中额外修掉的骨架/工具链问题**（均已提交在 `lab4` 分支）

1. `default_pmm.c` 引用了不存在的 `<best_fit_pmm.h>`
2. `kmalloc.c` 把 `unsigned long` 传给 `kva2page(void *)` —— 新版 GCC 报错
3. `Makefile` / `grade.sh` 的 `-device loader` 在 QEMU 8 下失效，改 `-kernel`
4. 启动页表缺低 1G 恒等映射 —— 开分页瞬间取指页错误
5. `tools/test_lab4.c` 的 `test_lab4_thread` 返回 `void` 却传给 `int (*)(void *)` —— GCC 14 起视为错误
6. `get_pte()` 中间级页表项误置 `PTE_U` —— RISC-V 规定非叶子 PTE 的 U 位必须为 0
