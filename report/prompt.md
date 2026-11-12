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

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：补回 lab3 的 `interrupt_handler()` 与 `exception_handler()`

> 沿用 lab3 的提示词（见 lab3 分支 `report/prompt.md` 第二节「提示词 1」「提示词 2」），
> 填入 `kern/trap/trap.c` 的 `//lab3 请补充你在lab3中的代码`。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 2：实现进程创建与初始化

> 涉及 `alloc_proc` / `proc_init` / `kernel_thread` / `do_fork` / `copy_thread` / `copy_mm` 六个函数（`kern/process/proc.c` 的 `//LAB4 function2 YOUR CODE`）。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

> **待填写（负责人：孙锘梵）**

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
