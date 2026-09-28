# 实验环境搭建指南（在自备电脑上）

> 适用对象：需要在自己电脑上做实验的组员。
> 目标：从零装好 WSL2 + Ubuntu + RISC-V 工具链 + QEMU，能编译运行 lab1，并能向仓库提交。
> 预计耗时：30~60 分钟，其中大部分是下载等待时间。

---

## ⚠️ 开始之前：先测一下你能不能访问 GitHub

**本项目的仓库在 `github.com`，而部分校园网（教育网）会把 `github.com:443` 拦截。** 组长所在的网络就属于这种情况。

**在动手装环境之前，先花一分钟测一下**（在任意能上网的地方执行）：

```bash
git ls-remote https://github.com/YSY-yt/os-lab-2026 HEAD
```

- **有输出**（一行 40 位的哈希值）→ 网络没问题，继续往下做。
- **卡住不动或报 `Could not resolve host` / `Failed to connect`** → 你的网络访问不了 GitHub。此时有三个选择：
  1. 开代理/VPN（如果有），并让 git 走代理：
     ```bash
     git config --global http.proxy http://127.0.0.1:端口号
     git config --global https.proxy http://127.0.0.1:端口号
     ```
  2. 用手机热点试（有时能通）
  3. **告诉组长**，由组长决定是否把仓库镜像到 Gitee

> 如果克隆/推送一直失败，不要自己反复重试，直接反馈给组长。

---

## 第一步：安装 WSL2 + Ubuntu 24.04

如果你电脑上**已经有**能用的 WSL + Ubuntu（20.04 或更新版本），可以跳到第二步。

**1. 以管理员身份打开 PowerShell**

按 `Win` 键 → 输入 `PowerShell` → 右键「Windows PowerShell」→ **以管理员身份运行**

**2. 执行安装命令**

```powershell
wsl --install -d Ubuntu-24.04
```

如果提示 `wsl` 命令不存在，先执行 `wsl --install`（不带参数），重启电脑后再执行上面这条。

**3. 重启电脑**（如果安装过程要求）

**4. 首次启动 Ubuntu 并设置用户名密码**

按 `Win` 键 → 搜索 `Ubuntu` → 点开。第一次会要求你设置：

- **用户名**：建议用英文小写，例如 `ysy`
- **密码**：输入时屏幕**不会显示任何字符**，这是正常的，输完按回车；会要求再输一次确认

> **请记住这个密码**，后面 `sudo` 命令要用。

---

## 第二步：安装工具链（一条命令）

打开 Ubuntu 终端（按 `Win` 搜 `Ubuntu`，或 Windows Terminal 下拉菜单选 `Ubuntu-24.04`），执行：

```bash
sudo apt-get update
sudo apt-get install -y git build-essential gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf qemu-system-misc gdb-multiarch
```

会提示输入密码（就是第一步设的那个，输入时不显示字符），输完回车。然后等待下载安装完成。

**这条命令装了什么：**

| 包 | 提供 | 用途 |
|---|---|---|
| `git` | `git` | 版本控制 |
| `build-essential` | `make`、`gcc` | 构建工具 |
| `gcc-riscv64-unknown-elf` | `riscv64-unknown-elf-gcc` | **交叉编译器**（前缀正好是 Makefile 需要的） |
| `binutils-riscv64-unknown-elf` | `riscv64-unknown-elf-ld` / `objcopy` / `objdump` | 链接与二进制工具 |
| `qemu-system-misc` | `qemu-system-riscv64` | **模拟器** |
| `gdb-multiarch` | `gdb-multiarch` | **调试器**（apt 里没有 `riscv64-unknown-elf-gdb`，用这个替代） |

> **关于版本差异**：apt 装的 GCC 是 13.2，组长机器上是 15.1.0。代码在更严格的新版 GCC 上能零警告通过 `-Werror`，所以 13.2 也能正常编译。万一真遇到 `-Werror` 报错，把完整报错发群里。

---

## 第三步：验证工具链

```bash
riscv64-unknown-elf-gcc -v
qemu-system-riscv64 --version
gdb-multiarch --version
```

三条命令都应输出版本信息（gcc 会输出一大段，**最后一行**有 `gcc version 13.2.0` 字样即成功）。

如果提示 `command not found`，说明第二步没装成功，重新执行第二步并留意报错。

---

## 第四步：克隆仓库

**建议克隆到 WSL 自己的家目录**（比放在 `/mnt/c`、`/mnt/d` 这些 Windows 盘上快很多，也少权限麻烦）：

```bash
cd ~
git clone https://github.com/YSY-yt/os-lab-2026.git
cd os-lab-2026
git checkout lab1
```

克隆下来的目录结构：

```
os-lab-2026/
├── README.md
├── docs/          # 分工说明与本指南
├── code/          # 实验代码
└── report/        # 报告（report.md / prompt.md / images/）
```

> **必须先执行 `git checkout lab1`**，因为默认分支是 `main`，而 lab1 的成果都在 `lab1` 分支上。

---

## 第五步：编译并运行，验证环境

```bash
cd ~/os-lab-2026/code
make
make qemu
```

**预期结果**：先打印一堆编译信息（`+ cc ...`、`+ ld ...`），然后 QEMU 启动，打印 OpenSBI 的 banner，最后出现：

```
(THU.CST) os is loading ...
```

**然后停住不动——这是完全正常的**，内核最后是 `while(1);` 死循环。

**退出 QEMU 的方法**：先按住 `Ctrl` 再按 `a`，两个键都松开，然后单独按一下 `x`。

看到 `(THU.CST) os is loading ...` 就说明你的环境**完全就绪**了。

### 如果没看到那行输出

| 现象 | 原因与解决 |
|---|---|
| 只有 OpenSBI banner，没有 `(THU.CST)` | 你的 `code/Makefile` 是旧版本。执行 `git pull` 拉取最新代码（该问题已修复） |
| `riscv64-unknown-elf-gcc: not found` | 第二步没装成功，重装 |
| `make: command not found` | `sudo apt-get install -y build-essential` |
| 提示 `riscv64-unknown-elf-gdb: not found` | `code/Makefile` 是旧版本，`git pull` 拉最新（已改为自动探测） |

---

## 第六步：配置 Git 身份并提交

**1. 设置你的提交身份**（只需一次）

```bash
cd ~/os-lab-2026
git config user.name "你的姓名"
git config user.email "你的邮箱"
```

**2. 确认你已被加为仓库协作者**

⚠️ **这一步必须由组长操作。** 仓库虽然是公开的，但「公开」只意味着**别人能查看和克隆**，**不能推送**。只有被加为 collaborator 的人才能 `git push`。

如果没被加，推送会报 `403` 或 `Permission denied`。**遇到这种情况直接找组长，不要自己反复试。**

**3. 测试推送**

```bash
cd ~/os-lab-2026
git pull
# 随便改一个地方测试，例如在 report/report.md 里加一行备注
git add -A
git commit -m "lab1: 测试提交"
git push
```

推送成功后刷新 GitHub 页面能看到你的提交，就说明通了。

---

## 日常工作流

```bash
cd ~/os-lab-2026

git pull                      # 1. 先拉取别人的改动
# ... 编辑文件（例如 report/report.md）...
git status                    # 2. 看看改了什么
git add -A                    # 3. 暂存
git commit -m "lab1: 说明你改了什么"   # 4. 提交
git push                      # 5. 推送
```

**三条规矩：**

1. **所有改动提交到 `lab1` 分支**，不要提交到 `main`（`git branch` 可以确认当前分支）。
2. `lab1` 分支根目录下必须是 `code/` 和 `report/` 两个文件夹，不要在里面乱建目录。
3. 报告里的图片用**相对路径**引用，例如 `./images/lab1_gdb.png`。

---

## 附：这次用到的主要命令速查

```bash
# 打开 Ubuntu
wsl -d Ubuntu-24.04                     # 或在开始菜单搜 Ubuntu

# 编译运行
cd ~/os-lab-2026/code
make                                    # 只编译
make qemu                               # 编译并运行（退出：Ctrl+A 松开再按 x）
make clean                              # 清理编译产物

# 调试（需要两个终端）
make debug                              # 终端 A：QEMU 暂停并开放 1234 端口
make gdb                                # 终端 B：GDB 连接

# 查看内核信息
riscv64-unknown-elf-objdump -d bin/kernel | grep -A6 "<kern_entry>:"
riscv64-unknown-elf-nm bin/kernel | grep -E "kern_entry|kern_init|bootstack|edata|end"

# 清理卡住的 QEMU
pkill -f qemu-system-riscv64
```

---

## 附：两条「不要踩」的坑

1. **不要用 `watch *0x80200000`。** 指导书推荐了这个监视点，但在本环境中**不会触发**。原因：内核镜像是 QEMU 在机器初始化阶段就写入内存的，OpenSBI 并不负责加载它，所以连接 GDB 时该地址上已经存在内核指令，之后不会再发生写入。请用 `b *0x80200000` 断点验证控制权移交。

2. **必须用交互式 Ubuntu 终端跑 `make`。** 如果写脚本用 `bash -lc` 调用，可能因为拿不到工具链 `PATH` 而报 `riscv64-unknown-elf-gcc: not found`。手动在终端里操作不会有这个问题。
