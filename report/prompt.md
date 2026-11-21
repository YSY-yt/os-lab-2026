# Lab 5 提示词汇总

> **本文件的组织方式**
>
> 本文件按小组成员分部分汇总本实验使用过的提示词：
>
> - **部分一：用户态系统调用** —— 负责人 2413628-黄乐仁
> - **部分二：创建加载第一个用户进程** —— 负责人 2410868-孙锘梵
> - **部分三：复制用户进程 + 其余系统调用** —— 负责人 2412796-杨斯源（组长）
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
> **lab5 的特点**：本实验依赖 lab2/3/4，代码里有**八处** `//lab2` / `//lab3` / `//lab4` 空缺需要补回；此外 lab3 的 `RESTORE_ALL` 只适用于纯内核系统，本实验要求**扩展为用户态/内核态双向返回**。补回时注意：物理内存管理器叫 `default_pmm_manager`，内部仍是 Best-Fit。

---

## 一、部分一：用户态系统调用

### 提示词 0：扩展 `RESTORE_ALL` 支持用户态返回

> 在 lab3 的 `RESTORE_ALL` 基础上修改 `kern/trap/trapentry.S`，使其能从用户态 trapframe 正确恢复（重点：用户 `sp`、页表根、`sstatus` 的 SPP/SPIE）。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：实现 `syscall()` 与九个 `sys_*`

> 涉及 `kern/syscall/syscall.c` 的 `//lab5 function1 YOUR CODE`，以及 `kern/trap/trap.c` 中 `exception_handler` 的 `CAUSE_USER_ECALL` 分支。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

> **待填写（负责人：黄乐仁）**

---

## 二、部分二：创建加载第一个用户进程

### 提示词 0：更新 `alloc_proc()` 与 `kernel_thread()`

> 在 lab4 的基础上，为 lab5 的字段（`exit_code` / `wait_state` / 进程树指针）和用户进程创建做准备。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：实现 `init_main()` / `user_main()` 与 `do_execve()` / `load_icode()`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 2：实现 `kernel_execve()` 与 `do_wait()`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

> **待填写（负责人：孙锘梵）**

---

## 三、部分三：复制用户进程 + 其余系统调用

### 提示词 0：实现 `do_fork()` / `copy_mm()` / `dup_mmap()` / `copy_range()`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：实现 `do_exit()` / `do_yield()` / `do_kill()`

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
