# Lab 3 提示词汇总

> **本文件的组织方式**
>
> 本文件按小组成员分部分汇总本实验使用过的提示词：
>
> - **部分一：Trap 上下文恢复与 Trap 入口** —— 负责人 [待定]
> - **部分二：中断与异常处理** —— 负责人 [待定]
> - **部分三：收口与验证** —— 负责人 [待定]
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
> **lab3 的特点**：本实验依赖 lab2，代码中有两处 `//lab2 请补充你在lab2中的代码`，需要**用你此前设计的提示词重新生成 lab2 的代码并填回**（见各部分的「提示词 0」）。此外两个功能模块都是汇编/底层处理，给 AI 的 `[RELY]` 必须包含**完整的 `SAVE_ALL` 与 `struct trapframe` 布局**——这是本实验最容易出错的地方。

---

## 一、部分一：Trap 上下文恢复与 Trap 入口

### 提示词 0：补回 lab2 的 `page_init()`

> 沿用 lab2 使用过的 `page_init` 提示词（见 lab2 分支的 `report/prompt.md` 第一节），
> 把生成结果填入 `kern/mm/pmm.c` 中标有 `//lab2 请补充你在lab2中的代码` 的位置。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：实现 `RESTORE_ALL` 与 `__alltraps`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

[待填写]

---

## 二、部分二：中断与异常处理

### 提示词 0：补回 lab2 的 Best-Fit 模块

> 沿用 lab2 使用过的 Best-Fit 提示词（见 lab2 分支的 `report/prompt.md` 第二节），
> 把生成结果填入 `kern/mm/best_fit_pmm.c` 中标有 `//lab2 请补充你在lab2中的代码` 的位置。
> **注意**：`basic_check` 与 `best_fit_check` 两个测试函数本实验已提供，**不要重复实现**。

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

### 提示词 1：实现 `interrupt_handler()` 与 `exception_handler()`

````markdown
[PROMPT]

[RELY]

[GUARANTEE]

[SPECIFICATION]
````

**生成的代码出了什么问题、怎么解决的：**

[待填写]

---

## 三、部分三：收口与验证

本部分负责报告汇总、格式校对与最终验证，**没有代码生成任务**，因此没有对应的提示词。

本部分的工作内容见 `docs/lab3-tasks.md` 第三节。

---

## 四、迭代记录

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|------|--------|-----------|-------------------|
| 1 | | | |
| 2 | | | |
| 3 | | | |
