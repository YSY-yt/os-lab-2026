# 操作系统实验报告

## 实验基本信息

| 项目 | 内容 |
|------|------|
| **实验名称** | Lab 5: 用户程序（系统调用 / 第一个用户进程 / 进程复制 / 进程控制） |
| **小组成员** | 2412796-杨斯源（组长）、2413628-黄乐仁、2410868-孙锘梵 |
| **完成日期** | 待填写 |

### 小组分工

练习如何分工？

| 成员 | 负责的练习/模块 |
|------|----------------|
| 2412796-杨斯源（组长） | 部分三：复制用户进程（`do_fork` / `copy_mm` / `dup_mmap` / `copy_range`）+ 其余系统调用（`do_exit` / `do_yield` / `do_kill`）+ 收口与验证 |
| 2413628-黄乐仁 | 部分一：用户态系统调用（`RESTORE_ALL` 扩展 / `exception_handler` / `syscall` / 九个 `sys_*`） |
| 2410868-孙锘梵 | 部分二：创建加载第一个用户进程（`init_main` / `user_main` / `kernel_execve` / `do_execve` / `load_icode` / `do_wait`） |

实验报告如何分工？

报告的撰写按上表划分：各功能模块由其负责人撰写对应章节；实验环境、总结与收获及 `prompt.md` 的汇总由部分三负责人负责；最后经统一校对格式与图片引用后提交。

---

## 一、实验目的

本实验的主要目的是：

1. **理解用户态与内核态的切换**：掌握 `ecall` 触发的用户态系统调用如何进入内核、`exception_handler` 如何识别 `CAUSE_USER_ECALL`、以及返回时如何从内核态 `sret` 回用户态。
2. **掌握系统调用的分发与实现**：理解 `syscall()` 如何从 `trapframe` 取系统调用号与参数、九个 `sys_*` 如何把统一编号翻译成具体内核语义，以及返回值如何写回用户寄存器。
3. **掌握 ELF 程序的装载**：理解 `load_icode()` 如何解析 ELF、建立用户地址空间（`mm_map` / `page_insert`）、装代码段与数据段、建用户栈并设置 `trapframe`。
4. **理解进程复制**：掌握 `do_fork` / `copy_mm` / `dup_mmap` / `copy_range` 如何按 VMA 粒度、再按页粒度复制父进程的地址空间。
5. **理解进程生命周期**：掌握 `do_exit` / `do_wait` / `do_yield` / `do_kill` 如何配合父子关系、僵尸态与调度器完成退出与回收。

---

## 二、实验环境

| 成员 | AI 编程工具 | 底层模型 | 备注 |
|------|------------|---------|------|
| 2412796-杨斯源 | 待填写 | 待填写 | |
| 2413628-黄乐仁 | 待填写 | 待填写 | |
| 2410868-孙锘梵 | 待填写 | 待填写 | |

**运行环境：**

| 组件 | 版本 / 包名 |
|------|-------------|
| 操作系统 | 待填写 |
| 交叉编译器 | 待填写 |
| 模拟器 | 待填写 |
| 调试器 | 待填写 |

**本实验做的环境适配**（与 lab2~lab4 同源，已提交在 `code/` 内）：

| 位置 | 改动 | 原因 |
|---|---|---|
| `Makefile`、`tools/grade.sh` | `-device loader,file=...,addr=0x80200000` → `-kernel bin/ucore.img` | QEMU ≥ 8 自带 OpenSBI 为 fw_dynamic，跳转地址必须由 QEMU 告知，否则内核不执行 |
| `kern/init/entry.S` | 启动页表补回低 1G 恒等映射 | 打开分页瞬间 PC 仍在物理地址 `0x8020xxxx`，不映射会立即取指页错误 |
| `kern/mm/kmalloc.c` | `kva2page(kva)` → `kva2page((void *)kva)` | 新版 GCC 下 `-Wint-conversion` 直接报错 |
| `Makefile` | GDB 命令自动探测 `riscv64-unknown-elf-gdb` / `gdb-multiarch` | apt 仓库没有前者 |
| `tools/grade.sh` | 不设 GDB 断点，轮询串口日志、输出稳定后收尾 | 原脚本的断点地址按老平台约定推导，在 riscv64 上不可用 |

---

## 三、实验整体逻辑分析

### 3.1 本章节的逻辑主线

> **待填写（负责人：杨斯源）**
> 建议围绕：本章在 lab4 的内核线程之上加入"用户态"，四条主线——
> 系统调用（用户 → 内核）、第一个用户进程（内核线程 → 用户进程）、进程复制（fork）、进程退出与回收。
> 关键链路：`ecall → __alltraps → SAVE_ALL → trap → exception_handler → syscall → sys_* → do_* → __trapret → sret`。

### 3.2 功能的逐步实现

> **待填写（负责人：杨斯源）**
> 建议按"为什么是这个顺序"写：先补回 lab2/3/4 代码 → 扩展 `RESTORE_ALL` 支持用户态返回（否则回不到用户态）→ 系统调用入口与分发 → ELF 装载与第一个用户进程 → fork 复制地址空间 → 退出/等待/让出/终止 → 验证。

---

## 四、实验内容与实现

### 前置：补回 lab2 / lab3 / lab4 的代码

**负责人：** 2413628-黄乐仁、2410868-孙锘梵

本实验代码里有八处标注了 lab2/lab3/lab4 的空缺，需要把此前实验的实现填回：

| 位置 | 需要补回的内容 | 建议负责人 |
|---|---|---|
| `kern/mm/default_pmm.c` | lab2 的 Best-Fit 管理模块（管理器名 `default_pmm_manager`） | 孙锘梵 |
| `kern/mm/pmm.c`（`page_init` 处） | lab2 的 `page_init()` | 黄乐仁 |
| `kern/mm/pmm.c`（`page_insert/page_remove` 处） | lab4 的 `page_insert()` / `page_remove()` | 黄乐仁 |
| `kern/trap/trap.c` | lab3 的两个 handler（本实验还要按功能模块一扩展 `exception_handler`） | 黄乐仁 |
| `kern/trap/trapentry.S` | lab3 的 `RESTORE_ALL` / `__alltraps`（**本实验要求扩展为用户态/内核态双向返回**） | 黄乐仁 |
| `kern/process/proc.c` | lab4 的进程创建相关代码（`proc_init` / `proc_run` / `cpu_idle` / `copy_thread` 等） | 孙锘梵 |
| `kern/process/switch.S` | lab4 的 `switch_to` | 杨斯源 |
| `kern/schedule/sched.c` | lab4 的 `schedule` | 杨斯源 |

> **注意**：`RESTORE_ALL` 不能原样照搬 lab3。lab3 的版本只适用于"纯内核、无用户态"的系统；本实验要求它支持用户态/内核态双向异常返回（用户态的 `sp`、页表根等都要正确恢复）。

---

### 功能模块一：用户态系统调用功能

**负责人：** 2413628-黄乐仁

**需要实现/修改的函数：**

```c
void exception_handler(struct trapframe *tf);   // kern/trap/trap.c（在 lab3 基础上扩展 CAUSE_USER_ECALL）
void syscall(void);                             // kern/syscall/syscall.c
static int sys_exit/fork/wait/exec/yield/kill/getpid/putc/pgdir(uint64_t arg[]);
```

**功能说明：** > **待填写（负责人：黄乐仁）**

**注意事项（指导书原文要点）：**

- 系统调用号放在 `a0`，参数放在 `a1`~`a5`，必须与 `user/libs/syscall.c` 的封装一致。
- `exception_handler` 识别到 `CAUSE_USER_ECALL` 后**必须把 `epc` 前移**，否则返回后重复执行同一条 `ecall`。
- `syscall()` 的分发表要做边界检查，非法编号返回 `-E_INVAL`，不能越界访问。
- 涉及用户指针的参数必须用 `user_mem_check` 校验，不能直接信任。
- `RESTORE_ALL` 要能从用户态 trapframe 正确恢复（含用户 `sp`）。

**验收标准：**

- `[Test Syscall] PASS`（进程创建测试之后由 `init_main` 开头注入）
- 各用户程序（`hello`、`badarg` 等）能正确运行并输出

**最终提示词：** > **待填写：见 `report/prompt.md` 第一节，并在此粘贴定稿原文。**

**实现迭代过程：** > **待填写（负责人：黄乐仁）**

---

### 功能模块二：创建加载第一个用户进程

**负责人：** 2410868-孙锘梵

**需要实现的函数：**

```c
static int init_main(void *arg);
static int user_main(void *arg);
static int kernel_execve(const char *name, unsigned char *binary, size_t size);
int do_execve(const char *name, size_t len, unsigned char *binary, size_t size);
static int load_icode(unsigned char *binary, size_t size);
int do_wait(int pid, int *code_store);
```

**功能说明：** > **待填写（负责人：孙锘梵）**

**注意事项（指导书原文要点）：**

- `kernel_execve` 不能只调 `do_execve`，还必须**构造返回用户态的路径**（否则只换了地址空间，没有真正进入用户态）。
- `load_icode` 要正确设置用户入口 `epc`、用户栈指针、`sstatus.SPP = 0`，否则 `sret` 回不到用户态。
- `do_execve` 必须先回收旧 `mm`（`exit_mmap` / `put_pgdir` / `mm_destroy`），再装载新程序。
- `do_wait` 要与僵尸态、`WT_CHILD`、唤醒逻辑一致，否则 `init_main` 永远等不到。

**验收标准：**

- `[Test Process Create] PASS`
- `kernel_execve: pid = 2, name = "..."` 与 `init check memory pass.` 出现

**最终提示词：** > **待填写：见 `report/prompt.md` 第二节，并在此粘贴定稿原文。**

**实现迭代过程：** > **待填写（负责人：孙锘梵）**

---

### 功能模块三：复制用户进程

**负责人：** 2412796-杨斯源（组长）

**需要实现的函数：**

```c
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);   // proc.c
static int copy_mm(uint32_t clone_flags, struct proc_struct *proc);         // proc.c
int dup_mmap(struct mm_struct *to, struct mm_struct *from);                 // vmm.c
int copy_range(pde_t *to, pde_t *from, uintptr_t start, uintptr_t end, bool share);  // pmm.c
```

**功能说明：** > **待填写（负责人：杨斯源）**

**注意事项（指导书原文要点）：**

- 普通 fork 要复制用户地址空间；内核线程路径（`current->mm == NULL`）不能去复制。
- `dup_mmap` 按 VMA 复制，保持区间顺序与权限；`copy_range` 按页复制内容并建立新映射。
- 页复制要用 `page_insert(..., PTE_U | ...)`，权限与源页一致；注意引用计数。
- `do_fork` 里 PCB、内核栈、trapframe、进程树、PID 都要正确建立，失败要回滚。

**验收标准：**

- `badarg` / `exit` / `forktest` 等用到 `fork` 的用户程序通过

**最终提示词：** > **待填写：见 `report/prompt.md` 第三节「提示词 0」，并在此粘贴定稿原文。**

**实现迭代过程：** > **待填写（负责人：杨斯源）**

---

### 功能模块四：其余系统调用

**负责人：** 2412796-杨斯源（组长）

**需要实现的函数：**

```c
int do_exit(int error_code);
int do_yield(void);
int do_kill(int pid);
```

**功能说明：** > **待填写（负责人：杨斯源）**

**注意事项（指导书原文要点）：**

- `do_exit` 不能让 `idleproc` / `initproc` 退出；要先释放用户地址空间，再置 `PROC_ZOMBIE` 并唤醒父进程。
- 退出、等待、僵尸回收三者必须状态机一致，否则出现僵尸无人回收或父进程永久睡眠。
- `do_yield` 置 `need_resched` 后交给调度器；`do_kill` 修改目标状态并视情况唤醒。
- 状态转换要在临界区内完成，不能留半更新状态。

**验收标准：**

- `[Test Process Control] PASS`
- `exit` / `spin`（kill 子进程）/ `yield` 等用户程序通过

**最终提示词：** > **待填写：见 `report/prompt.md` 第三节「提示词 1」，并在此粘贴定稿原文。**

**实现迭代过程：** > **待填写（负责人：杨斯源）**

---

### 扩展练习（Challenge）

> **待填写（负责人：全体，由部分三汇总）**
>
> 1. 说明该用户程序是何时被预先加载到内存中的？与我们常用操作系统的加载有何区别，原因是什么？
> 2. （选做，big challenge）在 ucore 中实现写时复制（COW）：父进程 fork 后共享只读页，任一进程写入时触发 page fault、复制页面。需要实现代码、测试用例与状态转换说明。

---

## 五、测试与验证

**编译与运行（`make qemu`）：**

> **待填写（负责人：杨斯源）**

![make qemu 运行结果](./images/lab5_qemu.png)

**自动评分（`make grade`）：**

`tools/grade.sh` 会依次构建并运行 13 个用户程序，每个程序检查内核自检输出与测试标记：

| 程序 | 检查要点 |
|---|---|
| `badsegment` / `divzero` / `softint` | 段错误、除零、软件中断异常处理 |
| `faultread` / `faultreadkernel` | 非法读不得 panic |
| `hello` / `testbss` / `pgdir` | 基本用户态运行、bss 清零、页表打印 |
| `yield` / `badarg` | 让出 CPU、非法参数返回错误 |
| `exit` / `spin` / `forktest` | 退出与等待、kill、fork 压力测试 |

> **待填写（负责人：杨斯源）**：填入实际得分与各 Test 标记结果。

![make grade 评分结果](./images/lab5_grade.png)

---

## 六、实验总结与收获

### 对操作系统的理解

**1. 本实验中重要的知识点，及与 OS 原理中对应知识点的关系**

> **待填写（负责人：杨斯源）**：建议覆盖——用户态/内核态与特权级、系统调用机制（ecall）、ELF 装载与地址空间、进程复制与写时复制、进程生命周期与僵尸态、父子进程与等待、调度与让出。

**2. OS 原理中很重要、但本实验没有对应上的知识点**

> **待填写（负责人：杨斯源）**

### AI 协作开发的经验

> **待填写（负责人：杨斯源）**

---

## 七、提交前自检

- [ ] 八处 lab2/lab3/lab4 代码已补回，全部 `lab5 functionN YOUR CODE` 已填并替换为学号
- [ ] `RESTORE_ALL` 已扩展为用户态/内核态双向返回
- [ ] `SAVE_ALL`、`struct trapframe`、`trap()` 签名未被修改
- [ ] `make` 编译通过且无警告，`make grade` 全部通过
- [ ] 全文无"待填写"；每个功能模块都有「最终提示词」与「实现迭代过程」
- [ ] `prompt.md` 三节齐全，迭代记录表已填
- [ ] `images/` 下截图齐全且为真实截图，路径为相对路径
- [ ] 成果在 `lab5` 分支，根目录只有 `code/` 与 `report/`
