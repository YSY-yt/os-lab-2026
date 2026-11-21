# Lab 5 任务分工说明

> 本文件是本小组的内部工作文档，用于明确 lab5 三个部分的负责人、具体工作内容与交付要求。
> 仓库：<https://github.com/YSY-yt/os-lab-2026>　分支：`lab5`
> 实验指导书：<http://oslab.mobisys.top/lab2026/_book/lab5/lab5.html>
> 当前状态：**准备阶段**（分支已建立、环境已适配，功能代码待实现）

---

## 零、先读这一节

### 0.1 分工总览

本实验有**四个功能模块**，按"系统调用链 / 第一个用户进程 / 进程复制与生命周期"归成三部分：

| | 部分一 | 部分二 | 部分三 |
|---|---|---|---|
| **主题** | 用户态系统调用 | 创建加载第一个用户进程 | 复制用户进程 + 进程控制 |
| **覆盖模块** | 功能模块一 | 功能模块二 | 功能模块三 + 功能模块四 |
| **要写/改的代码** | `trapentry.S` 的 `RESTORE_ALL`（扩展用户态返回）；`trap.c` 的 `exception_handler`；`syscall.c` 的 `syscall()` + 九个 `sys_*` | `proc.c`：`alloc_proc`/`kernel_thread` 更新、`init_main`、`user_main`、`kernel_execve`、`do_execve`、`load_icode`、`do_wait` | `proc.c`：`copy_mm`/`do_fork`/`do_exit`/`do_yield`/`do_kill`；`vmm.c`：`dup_mmap`；`pmm.c`：`copy_range` |
| **还要补回** | lab2 `page_init`、lab3 两个 handler、lab3 `RESTORE_ALL` | lab2 Best-Fit、lab4 的进程创建代码 | lab4 的 `switch_to`、`schedule`、`page_insert/page_remove` |
| **对应测试** | `[Test Syscall]` + `hello` 等用户程序 | `[Test Process Create]` + 需要 exec 的程序 | `[Test Process Control]` + `exit`/`spin`/`forktest` |
| **负责人** | 2413628-黄乐仁 | 2410868-孙锘梵 | 2412796-杨斯源（组长） |

**难度分布**：功能模块二最重（ELF 装载 + execve + wait），模块三/四次之，模块一函数多但逻辑直白。因此部分二只背一份 lab4 补回，部分一背两份、部分三背三份。

**选做**：Challenge 里"用户程序何时被预先加载"是**必答思考题**；COW 是 big challenge，默认不做，有余力再议。

### 0.2 提交规范（老师要求，不可违反）

- 一次实验一个分支：lab5 的成果**全部提交到 `lab5` 分支**，不要提交到 `main`。
- `lab5` 分支根目录下必须是两个文件夹：`code/` 与 `report/`。
- `report/` 下必须包含 `report.md`、`prompt.md`、`images/`。
- 报告图片一律用相对路径，例如 `./images/lab5_grade.png`。
- **保留 `labN` 与 `YOUR CODE` 注释，并把 `YOUR CODE` 替换为自己的学号。**

### 0.3 环境：已经替你们适配好了

`lab5` 分支的代码已预先修好五处，直接 `make` 即可：

| 已做的改动 | 原因 |
|---|---|
| `Makefile`、`tools/grade.sh`：`-device loader,...` → **`-kernel bin/ucore.img`** | QEMU ≥ 8 自带 OpenSBI 是 `fw_dynamic`，跳转地址必须由 QEMU 告知；否则内核永不执行 |
| `kern/init/entry.S`：启动页表补回**低 1G 恒等映射** | 打开分页瞬间 PC 还在物理地址 `0x8020xxxx`，这一页没映射会立即取指页错误 |
| `kern/mm/kmalloc.c`：`kva2page(kva)` → `kva2page((void *)kva)` | 新版 GCC 下 `-Wint-conversion` 直接报错 |
| `Makefile`：GDB 命令自动探测 `riscv64-unknown-elf-gdb` / `gdb-multiarch` | apt 仓库里没有前者 |
| `tools/grade.sh`：不设 GDB 断点，改为**轮询串口日志、输出连续 3 秒不增长就收尾** | 原脚本按老平台约定推导断点地址，riscv64 上不可用；`spin`/`forktest` 等程序运行时间差异大，固定 sleep 不可靠 |

**三条避坑提醒：**

1. **必须在交互式 Ubuntu 终端里跑 `make`**（非交互 shell 拿不到工具链 `PATH`）。
2. **退出 QEMU**：按住 `Ctrl` 再按 `a`，都松开后单独按 `x`。
3. **分支刚建好时编译不过**——八处 lab2/lab3/lab4 代码未补回、`syscall.c` 与 `proc.c` 的大片函数未实现。**这是预期的**，正好是任务本身。

### 0.4 自动评分是怎么打的

`make grade` 会依次构建并运行 **13 个用户程序**，每个程序都用 `default_check` 检查：

- **7 分**：内核启动自检字符串（`memory management: default_pmm_manager`、`check_alloc_page() succeeded!`、`check_pgdir() succeeded!`、`check_boot_pgdir() succeeded!`、`check_vma_struct() succeeded!`、`check_vmm() succeeded.`、`++ setup timer interrupts`）；
- **3 分**：三个测试标记与程序自身输出。

三个测试标记靠 `tools/inject.py` 注入，注入点很关键：

| 测试 | 注入位置 | 说明 |
|---|---|---|
| `[Test Process Create]` | `proc_init()` 末尾 | 检查 `idleproc`/`initproc` 字段 |
| `[Test Syscall]` | `init_main()` 开头 | 用合成 trapframe 直接调 `exception_handler` |
| `[Test Process Control]` | 同上 | 直接调 `do_yield` / `do_kill` / `do_wait` |

> ⚠️ **`proc_init` 与 `init_main` 必须存在且函数名不能改**，否则注入直接失败（`inject.py` 会报 "Function is missing"）。

**测试里的硬性断言**（直接决定实现细节，务必先看）：

```c
// Test Syscall
tf.cause = CAUSE_USER_ECALL; tf.epc = 0x1000; tf.gpr.a0 = SYS_getpid;
exception_handler(&tf);
assert(tf.epc == 0x1004);              // 必须把 epc 前移 4 字节
assert((int)tf.gpr.a0 == current->pid); // 返回值写回 a0
tf.gpr.a0 = 0;                          // 非法系统调用号 0
exception_handler(&tf);
assert((int)tf.gpr.a0 == -E_INVAL);     // 必须返回 -E_INVAL

// Test Process Control
assert(do_yield() == 0 && current->need_resched);
assert(do_kill(-1) == -E_INVAL);
assert(do_wait(-1, NULL) == -E_BAD_PROC);
```

**13 个用户程序**：`badsegment`、`divzero`、`softint`、`faultread`、`faultreadkernel`、`hello`、`testbss`、`pgdir`、`yield`、`badarg`、`exit`、`spin`、`forktest`。每个程序还额外要求 `! - 'user panic at .*'`——**用户态出错可以打印，但不能 panic 内核**。

> 每个程序 10 分（7 + 3），总分以 `make grade` 输出的 `Total Score` 为准。

### 0.5 三部分互相耦合

**（1）编译依赖**

- `syscall.c` 的 `syscalls[]` 表引用了九个 `sys_*`（部分一写）→ 未写则编译失败；
- `proc.c` 的 `init_main`/`kernel_execve`/`do_execve`/`load_icode`（部分二）与 `copy_mm`/`do_fork`/`do_exit`/`do_yield`/`do_kill`（部分三）都还是空的；
- `vmm.c::dup_mmap`（部分三）与 `pmm.c::copy_range`（部分三）；
- 八处 lab2/3/4 空缺未补 → 编译/链接不过。

**（2）运行时依赖链**

```
user 程序 ecall
  → __alltraps → SAVE_ALL → trap → exception_handler（部分一）
      → syscall → sys_*（部分一）
          → do_fork / do_execve / do_wait / do_exit / do_yield / do_kill（部分二、三）
  → __trapret → RESTORE_ALL（部分一，必须支持用户态）→ sret
```

**任何一环缺失，用户程序都跑不起来**，而现象往往是"内核 panic"或"进程卡死"，很难单看谁的代码判断。

**（3）谁改哪些文件**

| 部分 | 改动的文件 |
|---|---|
| 部分一 | `kern/trap/trapentry.S`、`kern/trap/trap.c`、`kern/syscall/syscall.c` |
| 部分二 | `kern/process/proc.c`（function2 区域）、`kern/mm/default_pmm.c`（补回） |
| 部分三 | `kern/process/proc.c`（function3/4 区域）、`kern/mm/vmm.c`、`kern/mm/pmm.c` |

部分二和部分三**都会改 `proc.c`**，但落在不同的 `//lab5 functionN` 标记区，冲突面较小。**提交前务必先 `git pull`。**

### 0.6 关于"补回 lab2/3/4 代码"

本实验代码里有八处空缺，需要按前几次实验的实现填回：

| 位置 | 补什么 | 建议负责人 |
|---|---|---|
| `kern/mm/default_pmm.c` | lab2 的 Best-Fit（管理器名 `default_pmm_manager`，测试函数不要重复实现） | 孙锘梵 |
| `kern/mm/pmm.c`（`page_init` 处） | lab2 的 `page_init()` | 黄乐仁 |
| `kern/mm/pmm.c`（`page_insert/page_remove` 处） | lab4 的两个映射函数 | 黄乐仁 |
| `kern/trap/trap.c` | lab3 的两个 handler | 黄乐仁 |
| `kern/trap/trapentry.S` | lab3 的 `RESTORE_ALL` / `__alltraps`（**本实验要扩展**） | 黄乐仁 |
| `kern/process/proc.c` | lab4 的进程创建代码（`proc_init`/`proc_run`/`cpu_idle`/`copy_thread` 等） | 孙锘梵 |
| `kern/process/switch.S` | lab4 的 `switch_to` | 杨斯源 |
| `kern/schedule/sched.c` | lab4 的 `schedule` | 杨斯源 |

> 前三份实验的代码都在对应分支的 `code/` 里，直接对照搬；注意 lab4 的 `copy_thread` 里 `sp` 的处理、`default_pmm_manager` 的命名。

---

## 一、部分一：用户态系统调用

**负责人：2413628-黄乐仁**

### 1.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| `RESTORE_ALL` 扩展 | `code/kern/trap/trapentry.S` | 能恢复用户态 trapframe（sp、sstatus、sepc） |
| `exception_handler` | `code/kern/trap/trap.c` | 增加 `CAUSE_USER_ECALL` 分支：`epc += 4` 后调 `syscall()` |
| `syscall()` | `code/kern/syscall/syscall.c` | 取号、取参、边界检查、分发、写回 `a0` |
| 九个 `sys_*` | 同上 | 映射到 `do_*` / `current->pid` / `cputchar` / `print_pgdir` |
| 报告「功能模块一」 | `report/report.md` | 含最终提示词、迭代过程 |
| 提示词 | `report/prompt.md` 第一节 | 四段式 |

### 1.2 实现要点

**`syscall()`**

```c
void syscall(void) {
    struct trapframe *tf = current->tf;
    uint64_t arg[5];
    int num = tf->gpr.a0;                    // 系统调用号在 a0
    if (num > 0 && num < NUM_SYSCALLS) {     // 注意 num=0 必须判为非法（测试断言）
        arg[0] = tf->gpr.a1; ... arg[4] = tf->gpr.a5;
        tf->gpr.a0 = syscalls[num](arg);     // 返回值写回 a0
    } else {
        tf->gpr.a0 = -E_INVAL;
    }
}
```

**九个 `sys_*` 的映射**

| 函数 | 实现 |
|---|---|
| `sys_exit` | `do_exit(arg[0])` |
| `sys_fork` | `do_fork(0, 0, current->tf)` |
| `sys_wait` | `do_wait((int)arg[0], (int *)arg[1])`，用户指针要 `user_mem_check` |
| `sys_exec` | `do_execve((const char *)arg[0], arg[1], (unsigned char *)arg[2], arg[3])`，`name` 要校验 |
| `sys_yield` | `do_yield()` |
| `sys_kill` | `do_kill((int)arg[0])` |
| `sys_getpid` | `current->pid` |
| `sys_putc` | `cputchar((int)arg[0])`，返回 0 |
| `sys_pgdir` | `print_pgdir()`，返回 0 |

**`exception_handler` 的 ecall 分支**

```c
case CAUSE_USER_ECALL:
    tf->epc += 4;      // ecall 是 4 字节指令，不前移会重复执行
    syscall();
    break;
```

**`RESTORE_ALL` 的扩展**：lab3 版本只适用于纯内核系统。本实验要保证从用户态 trapframe 恢复时：
- `sp` 最后恢复（用户栈指针）；
- `sstatus`、`sepc` 完整恢复（`SPP = 0` 时 `sret` 回到 U 态）；
- 不要用会被后续 `LOAD` 覆盖的寄存器传递中间值。

### 1.3 验收标准

- `[Test Syscall] PASS`
- `hello`、`badarg` 等程序能正确运行

### 1.4 完成标准（自检）

- [ ] `[Test Syscall]` 通过
- [ ] 非法系统调用号（含 0）返回 `-E_INVAL`
- [ ] `ecall` 返回地址前移 4 字节，不出现重复执行
- [ ] 用户指针参数经过 `user_mem_check`
- [ ] 注释保留、`YOUR CODE` 换成学号
- [ ] `prompt.md` 第一节提示词已填全

### 1.5 易错点

- ❌ 分发表判断写成 `num >= 0` → 0 号被当合法，测试直接失败。
- ❌ 忘记 `tf->epc += 4` → 用户程序在 `ecall` 处死循环。
- ❌ `syscall()` 里用函数参数 `tf` 而不是 `current->tf`（测试是直接调 `exception_handler(&tf)` 并设置 `current->tf`）。
- ❌ 直接信任用户传进来的指针 → 访问非法地址导致内核 panic（测试里有 `faultread` 等）。

---

## 二、部分二：创建加载第一个用户进程

**负责人：2410868-孙锘梵**

### 2.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| `alloc_proc` / `kernel_thread` 更新 | `code/kern/process/proc.c` | 增加 lab5 字段（`exit_code`、`wait_state`、进程树指针） |
| `init_main` | 同上 | 创建 `user_main` 内核线程，然后 `do_wait` 收尾 |
| `user_main` | 同上 | 选择要 exec 的用户程序并调用 `kernel_execve` |
| `kernel_execve` | 同上 | 调 `do_execve` 后**主动跳到 `__trapret`** 回到用户态 |
| `do_execve` / `load_icode` | 同上 | 回收旧 mm、解析 ELF、建地址空间与用户栈、设 trapframe |
| `do_wait` | 同上 | 等待子进程退出并回收 |
| 报告「功能模块二」 | `report/report.md` | 含最终提示词、迭代过程 |
| 提示词 | `report/prompt.md` 第二节 | 四段式 |

### 2.2 实现要点

**`user_main` 必须响应 `-DTEST`**（`make grade` 用 `build-<prog>` 传三个宏）：

```c
static int user_main(void *arg) {
#ifdef TEST
    KERNEL_EXECVE2(TEST, TESTSTART, TESTSIZE);
#else
    KERNEL_EXECVE(exit);
#endif
    panic("user_main execve failed.\n");
}
```

**`kernel_execve` 的关键**：只调 `do_execve` 不够——那只是把新映像装好，执行流仍在 S 态。要把当前进程"推"进通用返回路径：

```c
static int kernel_execve(const char *name, unsigned char *binary, size_t size) {
    int ret = do_execve(name, strlen(name), binary, size);
    if (ret != 0) return ret;
    struct trapframe *tf = current->tf;   // do_execve 已把入口/用户栈/sstatus 配好
    asm volatile("mv sp, %0\n\tj __trapret" :: "r"(tf));
    return 0;   // 不会到这里
}
```

**`load_icode` 的顺序**（决定了第一次能否回到用户态）：

1. 校验 ELF 头；
2. 对每个 `proghdr`：`mm_map` 建立 VMA → 分配页 → 拷贝段内容 → `page_insert`（`PTE_U | PTE_W | PTE_R | PTE_X`）；
3. 建立用户栈：`mm_map(USTACKTOP - USTACKSIZE, USTACKSIZE, VM_READ | VM_WRITE)`，并实际分配/映射栈页；
4. 设置 `current->tf`：`epc = elf->e_entry`、`gpr.sp = USTACKTOP`、`status` **清 SPP、置 SPIE、清 SIE**（返回 U 态并开中断）；
5. 切页表：`current->mm = mm; current->pgdir = mm->pgdir; write_csr(satp, ...); flush_tlb();`

**`do_execve`**：`exit_mmap(current->mm)` → `put_pgdir` → `mm_destroy` → 把 `current->mm/pgdir` 复位到内核页表 → `load_icode` → `set_proc_name`。

**`do_wait`**：按 `pid` 查子进程；没有子进程返回 `-E_BAD_PROC`；有子进程则 `current->wait_state = WT_CHILD` 后 `schedule()`，醒来再回收僵尸子进程并写 `code_store`。

### 2.3 验收标准

- `[Test Process Create] PASS`
- `kernel_execve: pid = 2, name = "..."` 与 `init check memory pass.` 出现

### 2.4 完成标准（自检）

- [ ] `[Test Process Create]` 通过
- [ ] 用户程序能从 ELF 入口开始执行（`hello` 打印 `Hello world!!.`）
- [ ] `kernel_execve` 之后确实进入 U 态（`sstatus.SPP = 0`）
- [ ] `do_execve` 前旧 mm 已完整回收
- [ ] 注释保留、`YOUR CODE` 换成学号
- [ ] `prompt.md` 第二节提示词已填全

### 2.5 易错点

- ❌ `kernel_execve` 只调 `do_execve` 就返回 → 内核线程继续在 S 态跑，用户程序永远不开始。
- ❌ `load_icode` 忘了给页设 `PTE_U` → 用户态访问触发 page fault。
- ❌ `tf->status` 没清 `SPP` → `sret` 回到 S 态而不是 U 态。
- ❌ `do_execve` 没回收旧 mm → 内存泄漏、旧页表残留。
- ❌ `do_wait` 的 `pid` 语义（`-1` 表示任意子进程）与测试断言不一致。

---

## 三、部分三：复制用户进程 + 其余系统调用

**负责人：2412796-杨斯源（组长）**

### 3.1 你要交付什么

| 交付物 | 位置 | 要求 |
|---|---|---|
| `copy_mm` | `code/kern/process/proc.c` | 内核线程路径 `mm = NULL`；用户进程路径复制地址空间 |
| `do_fork` | 同上 | 在 lab4 基础上接入 mm 复制、进程树、PID |
| `do_exit` / `do_yield` / `do_kill` | 同上 | 生命周期闭环 |
| `dup_mmap` | `code/kern/mm/vmm.c` | 按 VMA 复制 |
| `copy_range` | `code/kern/mm/pmm.c` | 按页复制内容并建映射 |
| 报告 3.1/3.2/五/六节、`prompt.md` 汇总 | `report/` | 收口 |
| 全量验证 | `make grade` + 截图 | 收口 |

### 3.2 实现要点

**`copy_range(to, from, start, end, share)`**

```c
for (; start < end; start += PGSIZE) {
    pte_t *ptep = get_pte(from, start, 0);
    if (ptep == NULL || !(*ptep & PTE_V)) continue;
    struct Page *page = pte2page(*ptep);
    struct Page *newpage = alloc_page();
    memcpy(page2kva(newpage), page2kva(page), PGSIZE);   // 复制内容
    page_insert(to, newpage, start, PTE_U | PTE_R | PTE_W | PTE_X);
}
```

**`dup_mmap(to, from)`**：遍历 `from->mmap_list` 的每个 VMA，在 `to` 里建立同样的区间（`mm_map`），再对该区间调 `copy_range`。

**`copy_mm`**：`current->mm == NULL` → `proc->mm = NULL; return 0;`；否则 `mm_create` + `setup_pgdir` + `dup_mmap`，失败要回滚。

**`do_fork`**：lab4 的流程 + `copy_mm` + `set_links`（父子/兄弟链）+ 关中断里 `get_pid`/`hash_proc`/`wakeup_proc`。

**`do_exit`**：不能杀 `idleproc`/`initproc`；先 `exit_mmap`/`put_mm`，再 `PROC_ZOMBIE`、`exit_code`，唤醒父进程，最后 `schedule()`。

**`do_yield`**：`current->need_resched = 1; return 0;`（测试断言返回值 0 且 `need_resched` 被置位）。

**`do_kill`**：`pid <= 0` 或找不到进程 → `-E_INVAL`；否则置 `PF_EXITING` 并 `wakeup_proc`。

### 3.3 验收标准

- `[Test Process Control] PASS`
- `exit`、`spin`、`forktest` 等程序通过

### 3.4 完成标准（自检）

- [ ] `[Test Process Control]` 通过
- [ ] `do_yield()` 返回 0 且置 `need_resched`
- [ ] `do_kill(-1) == -E_INVAL`、`do_wait(-1, NULL) == -E_BAD_PROC`
- [ ] `fork` 出的子进程有独立地址空间（父进程修改不影响子进程）
- [ ] 注释保留、`YOUR CODE` 换成学号

### 3.5 易错点

- ❌ `copy_range` 只复制映射不复制内容 → 父子进程共享物理页，一个改另一个也变。
- ❌ 忘记 `PTE_U` → 用户态访问新页 page fault。
- ❌ `do_exit` 释放了内核栈还继续用 → 崩溃；内核栈由父进程 `do_wait` 回收。
- ❌ `do_yield` 里直接 `schedule()` 并返回，导致 `need_resched` 没被置位（测试断言要求置位）。

---

## 四、收口清单（提交前逐条打勾）

### 代码

- [ ] `cd code && make clean && make` 编译通过且无警告
- [ ] `make qemu` 能进入用户态并运行默认程序
- [ ] `make grade` 全部通过（`Total Score` 达到满分）
- [ ] 八处 lab2/lab3/lab4 代码已补回，全部 `lab5 functionN` 已填并替换为学号
- [ ] `RESTORE_ALL` 已支持用户态返回
- [ ] `proc_init` / `init_main` 函数名未被修改（注入依赖）
- [ ] `bin/`、`obj/` 未被提交

### 报告

- [ ] 六节齐全，**全文无"待填写"**
- [ ] 四个功能模块都有「最终提示词」**和**「实现迭代过程」
- [ ] 报告要求第 4、5 条已覆盖
- [ ] 知识点对应表、未覆盖清单已写
- [ ] Challenge 思考题已答
- [ ] 三处「负责人」已署名，分工表与完成日期已填

### 提示词与图片

- [ ] `prompt.md` 三节齐全，迭代记录表已填
- [ ] `report/images/` 下有 `lab5_qemu.png` 与 `lab5_grade.png`

### 分支

- [ ] 成果在 `lab5` 分支，根下只有 `code/` 与 `report/`
- [ ] 已 `git push`

---

## 五、命令速查

```bash
cd ~/os-lab-2026/code          # 组长机器上是 /mnt/d/os-lab-2026/code

make                    # 编译（含 user/ 目录下的用户程序）
make qemu               # 运行（退出：Ctrl+A 松开再按 x）
make grade              # 自动评分（13 个用户程序）
make clean

make debug / make gdb   # 调试（两个终端）
pkill -f qemu-system-riscv64

git pull && git add -A && git commit -m "lab5: ..." && git push
```

**单独跑某个用户程序**（调试很有用）：

```bash
make build-hello && make qemu      # 内核会 exec hello
make build-forktest && make qemu
```

---

## 六、参考：lab5 官方要求原文

**实验目的**（`lab5_1_goals.html`）：

> 了解第一个用户进程创建过程；了解系统调用框架的实现机制；了解 ucore 如何实现系统调用 sys_fork/sys_exec/sys_exit/sys_wait 来进行进程管理。

**练习**（`lab5_2_1_exercises.html`）——四个功能模块，**均要求设计提示词借助 AI 完成**：

> **功能模块一：用户态系统调用功能**：`exception_handler`、`syscall`、九个 `sys_*`
>
> **功能模块二：创建加载第一个用户进程功能**：`init_main`、`alloc_proc`、`kernel_thread`、`do_wait`、`user_main`、`kernel_execve`、`do_execve`、`load_icode`
>
> **功能模块三：复制用户进程功能**：`do_fork`、`copy_mm`、`dup_mmap`、`copy_range`
>
> **功能模块四：其余系统调用功能**：`do_exit`、`do_yield`、`do_kill`

> 注意：原来 lab3 的 `RESTORE_ALL` 宏，它仅能用于纯内核、无用户态的系统。请在此基础上修改，扩充为支持用户态 / 内核态双向异常返回的完整版上下文恢复宏。

**扩展练习 Challenge**：

> - 实现 Copy on Write（COW）机制（big challenge，需要实现代码、测试用例与状态转换说明）
> - 说明该用户程序是何时被预先加载到内存中的？与我们常用操作系统的加载有何区别，原因是什么？

**报告要求**（`lab5_5_lab_requirement.html`）：

> 1. 基于 markdown 格式完成实验报告，以文本说明为主。
> 2. 说明自己对本章节整体逻辑线的理解，即本章节是按照什么顺序、围绕哪些功能逐步实现的。
> 3. 说明自己对每个功能的核心函数或功能模块的理解。
> 4. 说明每个练习的 spec 是怎么写的，prompt 是怎么写的。
> 5. 说明在使用提示词让 AI 生成代码的过程中出现了什么 bug，以及你是怎么解决的。
> 6. 列出你认为本实验中重要的知识点，以及与对应的 OS 原理中的知识点，并简要说明二者的含义、关系和差异。
> 7. 列出你认为 OS 原理中很重要、但在本实验中没有对应上的知识点。
>
> 注意有"lab5"的注释，代码中所有需要完成的地方（challenge 除外）都有"lab5"和"YOUR CODE"的注释，请在提交时特别注意保持注释，并将"YOUR CODE"替换为自己的学号。
