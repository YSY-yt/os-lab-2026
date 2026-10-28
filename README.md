# 操作系统课程实验（riscv64-ucore）

本仓库为操作系统课程小组实验的交付仓库，实验平台为 **riscv64-ucore**（最小可执行内核 → 物理内存 → 中断 → 虚拟内存 → 线程 → 进程 → 调度 → 同步互斥 → 文件系统 → 页面置换）。

实验指导书：<http://oslab.mobisys.top/lab2026/_book/>

仓库内的工作文档（在 `main` 分支）：

- **lab1 任务分工与交付要求**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/lab1-tasks.md>
- **lab1 答辩复习材料**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/lab1-review.md>
- **lab2 任务分工与交付要求**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/lab2-tasks.md>
- **lab2 部分三（Buddy System）设计思路**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/lab2-buddy-design.md>
- **lab2 收尾清单**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/lab2-closing-checklist.md>
- **lab3 任务分工与交付要求**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/lab3-tasks.md>
- **在自备电脑上搭建实验环境**：<https://github.com/YSY-yt/os-lab-2026/blob/main/docs/env-setup.md>

---

## 一、提交规范

**一个分支对应一次实验**，分支名为 `lab1`、`lab2`、……、`lab9`。

每个分支下包含两个文件夹：

```
labx/
├── code/                    # 本次实验实现后的源码
└── report/                  # 本次实验的报告材料
    ├── report.md            # 实验报告正文（Markdown）
    ├── prompt.md            # 本次实验使用的全部提示词汇总
    └── images/              # 报告中引用的测试截图
```

> 代码中形如 `//labN ... YOUR CODE` 的注释会按要求保留，并把 `YOUR CODE` 替换为组员学号。

---

## 二、实验进度

| 分支 | 实验主题 | 状态 |
|------|----------|------|
| `lab1` | 比麻雀更小的麻雀（最小可执行内核） | ✅ 已完成 |
| `lab2` | 物理内存和页表 | ✅ 已完成 |
| `lab3` | 断，都可以断（中断与异常） | 进行中 |
| `lab4` | 进程管理 | 未开始 |
| `lab5` | 用户程序 | 未开始 |
| `lab6` | 进程调度 | 未开始 |
| `lab7` | 同步互斥 | 未开始 |
| `lab8` | 文件系统 | 未开始 |
| `lab9` | 页面置换与内存映射 | 未开始 |

---

## 三、实验环境

在 WSL2 的 Ubuntu 24.04 上搭建：

| 组件 | 版本 / 包名 |
|------|-------------|
| 交叉编译器 | `riscv64-unknown-elf-gcc` 15.1.0 |
| 二进制工具 | `riscv64-unknown-elf-ld` / `objcopy` / `objdump` |
| 模拟器 | `qemu-system-riscv64` 8.2.2（apt 包 `qemu-system-misc`） |
| 调试器 | `riscv64-unknown-elf-gdb` 16.3.90（或 apt 的 `gdb-multiarch`） |

### 常用命令

```bash
make            # 编译内核
make qemu       # 编译并启动 QEMU 运行
make debug      # 启动 QEMU 并暂停，开放 GDB 端口 1234
make gdb        # 连接 GDB 调试
make grade      # 运行自动评分脚本（lab2 起提供）
```

### 本仓库对课程代码做过的环境适配

课程发放的 `Makefile` 与 `tools/grade.sh` 使用 `-device loader,file=...,addr=0x80200000` 启动 QEMU。该写法在 QEMU ≥ 8 下**无效**——QEMU 自带的 OpenSBI 已是 `fw_dynamic` 变体，跳转地址不再硬编码、必须由 QEMU 经 `-kernel` 告知，否则 `Domain0 Next Address` 为 `0x0`，内核永远不会被执行、评分为 0。因此统一改为 `-kernel`。

此外 `gdb` 目标原硬编码 `riscv64-unknown-elf-gdb`（apt 仓库无此包），已改为自动探测、回退 `gdb-multiarch`。

上述改动均在文件内以注释说明了原因，**未改动任何业务代码**。

---

## 四、小组成员

| 学号 | 姓名 | 角色 | 负责模块 |
|------|------|------|----------|
| 2412796 | 杨斯源 | 组长 | 见 `docs/lab3-tasks.md` |
| 2413628 | 黄乐仁 | 组员 | 见 `docs/lab3-tasks.md` |
| 2410868 | 孙锘梵 | 组员 | 见 `docs/lab3-tasks.md` |
