# Lab 1 答辩复习材料

> 面向 2412796-杨斯源（组长）。覆盖：你负责的部分、lab1 实验内容、Makefile、GDB、RISC-V 基础，以及本项目实际踩过的坑。
> **考前 5 分钟只看最后一节「关键数值速查表」和第一节的问答。**

---

## 一、最可能被问到的 15 个问题（附简洁答案）

| # | 问题 | 一句话答案 |
|---|---|---|
| 1 | lab1 要做成什么？ | 一个"比麻雀更小的麻雀"：能被 QEMU 加载、在 64 位 RISC-V 上真正执行、输出一行 `(THU.CST) os is loading ...` 然后死循环的最小可执行内核 |
| 2 | 内核被加载到哪个地址？ | `0x80200000` |
| 3 | 这个地址是谁规定的？ | `tools/kernel.ld` 里的 `BASE_ADDRESS = 0x80200000`；同时也必须和 OpenSBI 的跳转目标一致 |
| 4 | 为什么要自己写链接脚本？ | `ld` 默认脚本是为"跑在现有 OS 之上的应用程序"设计的，不适合链接内核；内核必须自己规定各段布局和装入地址 |
| 5 | 内核的第一条指令是什么？在哪？ | `kern_entry` 的 `la sp, bootstacktop`，位于 `0x80200000` |
| 6 | 为什么第一步要设 `sp`？ | OpenSBI 移交控制权时**没有为内核准备栈**，而接下来的 C 代码（函数调用、局部变量、保存寄存器）必须有栈 |
| 7 | `la` 是什么？展开成什么？ | 伪指令，展开为 `auipc sp, %pcrel_hi(bootstacktop)` + `addi sp, sp, %pcrel_lo(bootstacktop)` |
| 8 | `tail` 和 `call` 的区别？ | `tail` 展开为 `auipc`+`jalr`，且 `jalr` 目标是 **`x0`**，等于**丢弃返回地址**；`call` 会把返回地址存进 `ra` |
| 9 | 为什么用 `tail` 不用 `call`？ | `kern_init` 是 `__attribute__((noreturn))`，末尾 `while(1)`，永不返回；没必要在刚建好的空栈上压一个用不到的返回帧 |
| 10 | 加电后 CPU 从哪个地址取指？ | `0x1000`（RISC-V 复位地址），这段代码是**固件的复位向量，不是内核** |
| 11 | 复位向量那几条指令干了什么？ | 备好参数（hart id 放进 `a0`、设备树地址放进 `a1`），然后跳到 OpenSBI 所在的 `0x80000000` |
| 12 | OpenSBI 是什么？跑在哪个特权级？ | QEMU 自带的固件，运行在最高特权级 **M 模式**；负责最底层硬件初始化，然后把控制权交给 S 模式的内核 |
| 13 | 内核没有标准库，`cprintf` 怎么输出？ | 自底向上：`cprintf` → `vprintfmt`/`cputch` → `cons_putc` → `sbi_console_putchar` → `sbi_call` 里执行 `ecall` 陷入 M 模式请求 OpenSBI |
| 14 | `.bss` 段为什么要清零？ | `.bss` 在文件里只记录大小和位置、不存数据，装入内存时必须是 0；内核没有 runtime，所以要在 `kern_init` 里自己 `memset(edata, 0, end - edata)` |
| 15 | 你们这次遇到并解决了什么问题？ | 两件：① 发放的代码 zip 里行尾 CRLF 混装；② 原 Makefile 的 `-device loader` 在 QEMU 8.2 下失效，内核根本没被执行，改成 `-kernel` 才正常（详见第八节，**这是答辩加分点**） |

---

## 二、你负责的部分三（你的直接责任范围）

按 `docs/lab1-tasks.md`，你负责 **原理对应、提示词汇总与最终收口**：

| 交付物 | 具体内容 |
|---|---|
| 报告**第二节** 实验环境 | 三人的 AI 工具表 + 运行环境表 |
| 报告**第六节** 对操作系统的理解 | ① 6 条「本实验知识点 ↔ OS 原理知识点」对应表（含关系与差异）；② 6 条「原理中重要但本实验未覆盖」清单 |
| 报告**第六节** AI 协作开发的经验 | 4 个小节：环境问题、文档与实测、可独立验证的分工、必须亲自动手的环节 |
| `prompt.md` | 按成员分四节汇总 + 3 轮迭代记录 |
| 报告头 | 完成日期、小组分工表 |
| 收口 | 校对格式与图片引用、最终推送 |

**第六节的 6 条对应关系**（要能随口说出）：固件/引导加载程序、链接脚本与装入地址、内核栈、`.bss` 清零、`ecall` 与系统调用、freestanding 运行时。

**6 条未覆盖**：虚拟内存与分页、进程与线程抽象、中断与异常处理、并发与同步、文件系统与设备抽象、调度。

> 被问"为什么这些没覆盖"时的答法：lab1 的定位是**最小可执行内核**，它把 OS 的大部分机制**故意剥掉了**，只留启动与输出的骨架。这些正是后续 lab2~lab9 逐一补回来的内容。

---

## 三、lab1 实验内容与两个练习

### 3.1 项目文件结构

```
lab1/
├── Makefile                 # 构建入口
├── kern/
│   ├── init/entry.S         # 内核入口汇编（kern_entry）★
│   ├── init/init.c          # 内核 C 入口（kern_init）★
│   ├── driver/console.c     # cons_putc → sbi_console_putchar
│   ├── libs/stdio.c         # cprintf / cputs / cputchar
│   └── mm/{memlayout.h,mmu.h}  # KSTACKSIZE / PGSIZE 等宏
├── libs/                    # 内核自己的"库"（不是标准库）
│   ├── sbi.c/.h             # SBI 调用封装（sbi_call）
│   ├── printfmt.c           # 格式化输出实现
│   ├── string.c             # memset/memcpy 等
│   └── defs.h, riscv.h, ...
└── tools/
    ├── kernel.ld            # 链接脚本 ★
    └── function.mk          # Makefile 的函数库
```

### 3.2 练习1：理解内核启动中的程序入口操作

**题目**：`la sp, bootstacktop` 完成了什么操作、目的是什么？`tail kern_init` 完成了什么操作、目的是什么？

**`la sp, bootstacktop`**
- **操作**：`la`（load address）是伪指令，汇编器展开为两条真实指令：
  ```asm
  auipc sp, %pcrel_hi(bootstacktop)    # 取符号相对 PC 的高 20 位偏移，左移 12 位加到 PC
  addi  sp, sp,  %pcrel_lo(bootstacktop)  # 补低 12 位
  ```
  效果 = **把 `bootstacktop` 的绝对地址写入 `sp`**。
- **目的**：**建立内核栈**。`bootstack` 是 `.data` 段里 8192 字节的数组，`bootstacktop` 在它的**高地址端**；RISC-V 栈**向低地址增长**，把 `sp` 指到高地址端才是"空栈"，压栈时向低地址长，正好落在 `bootstack` 预留的空间内。
- **为什么必须**：OpenSBI 跳转过来时**没有给内核准备可用的栈**。实测断点命中瞬间 `sp = 0x80046eb0`——那是指向 OpenSBI 内存区的**固件残留值**，内核不能拿它当栈用。所以必须先自己立栈。

**`tail kern_init`**
- **操作**：`tail` 也是伪指令，展开为：
  ```asm
  auipc t1, %pcrel_hi(kern_init)
  jalr  x0, t1, %pcrel_lo(kern_init)   # 目标寄存器是 x0
  ```
  `jalr` 的语义是"跳转，同时把返回地址写入指定寄存器"。写进 **`x0`（零寄存器）意味着返回地址被丢弃**。效果 = **跳转到 `kern_init` 且不保存返回地址**。
- **目的**：把控制权从汇编入口交给 C 入口，这是"汇编阶段"与"C 阶段"的分界线。
- **为什么不用 `call`**：`kern_init` 声明为 `__attribute__((noreturn))`，末尾 `while(1)`，**永不返回**。既然后面没有代码要"回来"执行，就没必要在刚建好的空栈上压入一个永远用不到的返回地址帧。同时也让内核初始调用栈最干净——`kern_init` 就是最外层。

> **实测佐证**（能说出来更加分）：反汇编显示
> ```
> 80200000 <kern_entry>:
>   80200000: auipc sp,0x3        ← 0x80200000 + 0x3000 = 0x80203000
>   80200004: mv  sp,sp           ← 即 addi sp,sp,0（低位恰为 0，因为 bootstacktop 页对齐）
>   80200008: j   8020000a <kern_init>   ← 链接器把 tail 松弛成单条 jal，x0 仍表示丢弃返回地址
> ```

### 3.3 练习2：使用 GDB 验证启动流程

**题目**：用 GDB 跟踪从加电到内核第一条指令（跳转到 `0x80200000`）的整个过程；回答"RISC-V 硬件加电后最初执行的几条指令位于什么地址？主要完成了哪些功能？"

**调试过程**（两个终端）：

```bash
# 终端 A：QEMU 带 -S -s 启动，占住前台等待连接
make debug
# 终端 B：GDB 连接 1234 端口
make gdb
```

```gdb
info registers pc        # 复位后 pc = 0x1000
x/10i 0x1000             # 看复位向量
b *0x80200000            # 在控制权移交处下断点
continue                 # 命中 kern_entry
info registers pc sp     # pc=0x80200000，sp=0x80046eb0
si                       # 单步执行 la sp, bootstacktop
info registers pc sp     # pc=0x80200004，sp=0x80203000
```

**答案**：
- **地址**：加电后 CPU 从**复位地址 `0x1000`** 取指。这段代码**不是内核**，是 QEMU `virt` 机器固化在 MROM 里的**复位向量**，`0x1000~0x1014` 共 **6 条**会被执行的指令。
- **功能**：让机器达到"可以执行固件主体"的最小状态——**备好参数、然后跳转**：
  | 地址 | 指令 | 作用 |
  |---|---|---|
  | `0x1000` | `auipc t0,0x0` | `t0 = 0x1000`，作为取数的基址 |
  | `0x1004` | `addi a2,t0,40` | `a2 = 0x1028`，指向紧随其后的设备树数据 |
  | `0x1008` | `csrr a0,mhartid` | `a0` = 当前 hart id（启动参数） |
  | `0x100c` | `ld a1,32(t0)` | `a1` = 从 `0x1020` 读出的设备树 FDT 地址（启动参数） |
  | `0x1010` | `ld t0,24(t0)` | `t0 = 0x80000000`（OpenSBI 的加载地址） |
  | `0x1014` | `jr t0` | **跳转到 `0x80000000`，把控制权交给 OpenSBI** |
- **补充**：`0x1018` 之后 `x/10i` 会反汇编出 `unimp`，那些位置放的是**数据**（OpenSBI 地址、设备树信息），不是代码。真正的硬件初始化和"把内核放到 `0x80200000`"都是 OpenSBI 做的。

---

## 四、启动链路全流程（务必背熟）

```
① 加电
   CPU 的 PC 被硬件强制设为复位地址 0x1000
   → 执行 MROM 里的复位向量 6 条指令（备参数：a0=hartid, a1=FDT）
   → jr 跳转到 0x80000000

② OpenSBI（M 模式固件）
   地址：0x80000000（Firmware Base，可执行区间 0x80000000~0x8003ffff）
   → 完成最底层硬件初始化（中断控制器、时钟、串口等）
   → 内核镜像在 QEMU 开始执行前就已被写入 0x80200000
   → 跳转到 0x80200000，控制权移交内核（切到 S 模式）

③ 内核入口 kern_entry（0x80200000，kern/init/entry.S）
   → la sp, bootstacktop     建立内核栈（sp: 固件残留值 → 0x80203000）
   → tail kern_init          跳转 C 入口，丢弃返回地址

④ C 入口 kern_init（0x8020000a，kern/init/init.c）
   → memset(edata, 0, end - edata)   清 .bss 段
   → cprintf("(THU.CST) os is loading ...\n\n")
   → while (1);                      死循环
```

**关键理解**：控制权经过**两次跳转**（复位向量 → OpenSBI → 内核），每次都**不保证给下一级留下运行环境**。所以内核必须自己立栈——这就是练习1 那条 `la sp, bootstacktop` 存在的根本原因。

---

## 五、RISC-V 与汇编基础

### 5.1 特权级

| 级别 | 名称 | 谁在用 |
|---|---|---|
| **M**（Machine） | 最高特权级 | OpenSBI 固件 |
| **S**（Supervisor） | 内核特权级 | 我们的内核 |
| U（User） | 用户态 | lab1 用不到（还没有用户进程） |

> lab1 只用到 M/S 两级，**U 模式尚未登场**——这是"未覆盖"清单里可以提的点。

### 5.2 指令与伪指令

| 名称 | 展开/含义 |
|---|---|
| `la rd, symbol` | 伪指令 → `auipc` + `addi`，取符号地址 |
| `tail symbol` | 伪指令 → `auipc` + `jalr x0, ...`，尾调用，**丢弃返回地址** |
| `call symbol` | 伪指令 → `auipc` + `jalr ra, ...`，保存返回地址到 `ra` |
| `auipc rd, imm` | `rd = PC + (imm << 12)`，取 PC 相对高位 |
| `jalr rd, rs, imm` | 跳转到 `rs+imm`，返回地址写入 `rd`；**`rd=x0` 即丢弃** |
| `jal rd, offset` | 相对跳转；`jal x0, ...` 汇编器显示为 `j ...` |
| `csrr rd, csr` | 读控制状态寄存器，如 `mhartid` |
| `jr rs` | 跳转到 `rs`（即 `jalr x0, rs, 0`） |
| `ecall` | 环境调用，用于发起 SBI 请求（S → M） |

**寄存器别名**：`x0`=`zero`（恒为 0，写它等于丢弃）、`x2`=`sp`、`x1`=`ra`、`x10~x17`=`a0~a7`（参数/返回值）。

### 5.3 特权级切换的唯一入口：`ecall`

内核在 S 模式，要输出字符必须**请求 M 模式的 OpenSBI 帮忙**。`sbi_call` 就是干这个的：

```c
uint64_t sbi_call(uint64_t sbi_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    __asm__ volatile (
        "mv x17, %[sbi_type]\n"   // a7 = 功能号
        "mv x10, %[arg0]\n"       // a0 = 参数
        "mv x11, %[arg1]\n"       // a1
        "mv x12, %[arg2]\n"       // a2
        "ecall\n"                 // 陷入 M 模式
        "mv %[ret_val], x10"      // 返回值在 a0
        ...
    );
}
```

**调用约定**：功能号放 `x17`(a7)，参数放 `a0~a2`，返回值在 `a0`。

### 5.4 输出链路（自底向上）

```
cprintf("...")                kern/libs/stdio.c   格式化
  → vcprintf → vprintfmt      libs/printfmt.c     解析格式串
    → cputch(char, cnt)       kern/libs/stdio.c
      → cons_putc(c)          kern/driver/console.c
        → sbi_console_putchar sbi.c
          → sbi_call(1, ch,0,0)  ecall！
            → M 模式 OpenSBI 的 console_putchar → 串口(uart8250) → 屏幕
```

**为什么要这么绕**：内核没有标准库、也没有直接操作硬件的权限，**唯一的输出途径就是请求固件**。

---

## 六、Makefile 逐项详解

### 6.1 关键变量

| 变量 | 值 | 说明 |
|---|---|---|
| `GCCPREFIX` | `riscv64-unknown-elf-` | 交叉工具链前缀 |
| `CC` | `$(GCCPREFIX)gcc` | 交叉编译器 |
| `LD` | `$(GCCPREFIX)ld` | 链接器 |
| `OBJCOPY` | `$(GCCPREFIX)objcopy` | 用于生成纯二进制 |
| `OBJDUMP` | `$(GCCPREFIX)objdump` | 反汇编（我们用来佐证练习1） |
| `QEMU` | `qemu-system-riscv64` | 模拟器 |
| `GDB` | 自动探测 | 有 `riscv64-unknown-elf-gdb` 用它，否则退回 `gdb-multiarch` |
| `V` | `@` | 命令回显抑制 |

### 6.2 编译选项（`CFLAGS`）逐条解释

| 选项 | 作用 |
|---|---|
| `-mcmodel=medany` | 代码模型：任意位置寻址（内核被放在 `0x80200000`，不在 PC 附近也能寻址） |
| `-std=gnu99` | C 标准 |
| `-Werror` | **警告当错误**（这也是为什么换新版 GCC 要小心） |
| `-fno-builtin` | 不用编译器内置函数（因为 `memset` 等要自己实现） |
| `-Wall` `-Wno-unused` | 开大部分警告，但忽略未使用 |
| `-O2` | 优化等级 |
| `-nostdinc` | **不用标准头文件**，只用自己的 `-I` 目录 |
| `-fno-stack-protector` | 裸机没有栈保护机制 |
| `-ffunction-sections -fdata-sections` | 每个函数/变量独立成段，便于后面 `--gc-sections` 回收未用代码 |
| `-g` | 生成调试信息（GDB 要用） |

### 6.3 链接选项（`LDFLAGS`）

| 选项 | 作用 |
|---|---|
| `-m elf64lriscv` | 输出 64 位 RISC-V ELF |
| `-nostdlib` | 不链接标准库 |
| `--gc-sections` | 回收未被引用的段（配合上面的 `-ffunction-sections`） |
| `-T tools/kernel.ld` | **指定我们自己的链接脚本** |

### 6.4 主要目标

| 目标 | 作用 |
|---|---|
| `all` / `TARGETS`（默认） | 编译链接，产出 `bin/kernel` 和 `bin/ucore.img` |
| `qemu` | 编译 + 启动 QEMU 运行 |
| `debug` | 编译 + 启动 QEMU，带 `-s -S`（暂停 + 开放 gdb 端口） |
| `gdb` | 启动 GDB 连接 `localhost:1234` |
| `clean` | 清理编译产物 |
| `grade` | 从 ucore 模板继承的残留目标，调用 `tools/grade.sh`——**该脚本未随 lab1 发放，无法运行** |

### 6.5 构建三部曲（要能说清每步为什么需要）

```
① riscv64-unknown-elf-gcc -c ...   编译：.c/.S → .o（目标文件）
② riscv64-unknown-elf-ld -T kernel.ld ...   链接：.o → bin/kernel（ELF 可执行文件）
③ riscv64-unknown-elf-objcopy --strip-all -O binary  转换：bin/kernel → bin/ucore.img（纯二进制）
```

**为什么最后要 `objcopy`**：ELF 文件带头部、符号表、调试信息等元数据，**加载器只想要一段纯字节**。转换成 `-O binary` 后，内存布局被映射成一块线性数据，按链接脚本指定的地址装进去正好对上。

> 顺带：`objcopy --strip-all` 会剥掉符号表，所以 `bin/kernel` 有符号（能 `nm`），`bin/ucore.img` 没有。

### 6.6 我们改动过的两处（**答辩时如果被问到，一定要说清原因**）

| 改动 | 原来 | 现在 | 原因 |
|---|---|---|---|
| QEMU 启动方式 | `-device loader,file=$(UCOREIMG),addr=0x80200000` | **`-kernel $(UCOREIMG)`** | 前者在新版 QEMU 下无效（见第八节） |
| 调试器 | 硬编码 `riscv64-unknown-elf-gdb` | **自动探测** `riscv64-unknown-elf-gdb` → 回退 `gdb-multiarch` | apt 仓库没有前者这个包，只有后者 |

---

## 七、GDB 与本次调试要点

### 7.1 `make debug` 的两个参数（**很可能被问**）

| 参数 | 含义 |
|---|---|
| `-s` | 在 **1234 端口**开放 GDB 调试桩（等价于 `-gdb tcp::1234`） |
| `-S` | CPU 启动后**立即暂停**、不执行任何指令（`-S` = "freeze"），等 GDB 来接管 |

**为什么要两个终端**：`make debug` 会让 QEMU **一直占着前台**运行等待连接；GDB 是**另一个进程**去连它。所以必须先跑 `make debug`，再在另一个终端跑 `make gdb`。

### 7.2 常用 GDB 命令

| 命令 | 作用 |
|---|---|
| `target remote localhost:1234` | 连接 QEMU 的调试桩 |
| `set arch riscv:rv64` | 指定目标架构 |
| `file bin/kernel` | 载入符号表（这样才能看到函数名、源码行） |
| `info registers pc` / `pc sp` | 看寄存器 |
| `x/10i 0x1000` | 从某地址反汇编 10 条指令（`i`=指令） |
| `break *0x80200000` / `b *addr` | 在**绝对地址**下断点 |
| `continue` / `c` | 继续运行 |
| `si` | 单步执行**一条指令**（stepi），会跟进跳转 |
| `x/3i $pc` | 反汇编当前 PC 处 |
| `quit` | 退出 |

### 7.3 本次调试的三个阶段

| 阶段 | 命令 | 观察到的 |
|---|---|---|
| 一、复位 | `info registers pc`、`x/10i 0x1000` | `pc = 0x1000`，6 条复位向量指令 |
| 二、看内核是否已在内存 | `x/3i 0x80200000`、`b *0x80200000` | 该地址**已有内核指令**（QEMU 预先加载） |
| 三、抓控制权移交 | `continue`、`info registers pc sp`、`si` | 命中 `kern_entry`；`sp` 从 `0x80046eb0` → `0x80203000` |

### 7.4 为什么用 `b` 而不是 `watch`（**答辩加分点，一定要能讲**）

指导书建议用 `watch *0x80200000` 观察"内核加载的瞬间"。**实测这个监视点永远不触发。**

**原因**：内核镜像是 **QEMU 在机器初始化阶段就写入内存的**，不是 OpenSBI 在运行过程中加载的。所以 GDB 连接上的那一刻（CPU 被 `-S` 暂停在复位状态），`0x80200000` 上**已经存在完整的内核指令**，此后该地址不会再发生写入——监视"写入"自然无从触发。

**结论**：改用 `b *0x80200000` 断点，抓"控制权移交"那一刻，效果等价且可靠。

---

## 八、我们遇到并解决的两个问题（**答辩最有价值的部分**）

> 这部分讲的时候要按「**现象 → 定位 → 根因 → 解决**」的顺序，比空泛的感想有说服力得多。

### 问题一：代码包行尾 CRLF 混装

- **现象**：发放的 `lab1.zip` 解压后，`Makefile` 是 LF，其余 20 个文件却是 CRLF。
- **风险**：`tools/function.mk` 是被 `Makefile` `include` 进来的，CRLF 会让其中变量值**尾随一个 `\r`**，污染命令行；`tools/kernel.ld` 链接脚本也可能因此解析异常。
- **解决**：统一规范化为 LF，并加 `.gitattributes`（`* text=auto eol=lf`）把规则固化，避免后续 lab 重蹈。

### 问题二：Makefile 在新版 QEMU 下无法启动内核（**重点**）

- **现象**：`make qemu` 只打印 OpenSBI 的 banner，**看不到 `(THU.CST) os is loading ...`**。
- **定位**：在 `0x80200000` 下断点，发现**这个断点从头到尾没被命中过**——说明内核根本没被执行。回头看 OpenSBI 的输出，注意到一行关键信息：
  ```
  Domain0 Next Address      : 0x0000000000000000
  ```
  OpenSBI 准备跳转的地址是 **`0x0`**，不是 `0x80200000`。
- **根因**：原 Makefile 用 `-device loader,file=...,addr=0x80200000` 启动。这个参数**只是把内核字节拷进内存，并不设置"跳转目标"**。QEMU 4.x 时代自带的 OpenSBI 是 **`fw_jump`** 变体（跳转地址硬编码 0x80200000）所以能跑；QEMU 8.2 自带的已经换成 **`fw_dynamic`** 变体，跳转地址必须由 QEMU 通过 `-kernel` 等参数写入的 `fw_dynamic_info` 告知，否则就是 0。
- **验证**：改用 `-kernel bin/ucore.img` 后：
  ```
  Domain0 Next Address      : 0x0000000080200000     ← 正确了
  (THU.CST) os is loading ...                        ← 内核执行了
  ```
  再用 GDB 确认断点能命中 `kern_entry`。
- **解决**：把 `qemu` 和 `debug` 两个目标的启动参数改为 `-kernel $(UCOREIMG)`，并在 Makefile 里写注释说明原因。

### 问题三：指导书与实际环境不符

见 7.4。另外还有一处措辞问题：`Makefile` 里**有一个 `grade` 目标**（从 ucore 模板继承），但它调用的 `tools/grade.sh` **并未随 lab1 代码发放**，其引用的 `kern/trap/trap.c` 也不存在——所以 **lab1 没有自动评分脚本**，这一点在报告第五节里写清楚了。

---

## 九、关键数值速查表（考前 5 分钟看这个）

| 项 | 值 | 备注 |
|---|---|---|
| RISC-V 复位地址 | **`0x1000`** | 加电后 PC 初值 |
| 复位向量指令数 | **6 条**（`0x1000`~`0x1014`） | `0x1018` 起是数据 |
| OpenSBI 加载地址 | **`0x80000000`** | Firmware Base |
| **内核加载地址** | **`0x80200000`** | `BASE_ADDRESS`，也是 `kern_entry` |
| `kern_init` | `0x8020000a` | C 入口 |
| `bootstack` | `0x80201000` | 内核栈起始（页对齐） |
| **`bootstacktop`** | **`0x80203000`** | 栈顶；`la` 执行后 `sp` 的值 |
| `SBI_CONSOLE_PUTCHAR` | `0x80203000` | **`libs/sbi.c` 里的全局变量**，与 `bootstacktop` 同址 |
| `edata` = `end` | `0x80203008` | `.bss` 段长度为 **0** |
| `KSTACKSIZE` | **8192**（2 页） | `KSTACKPAGE=2 × PGSIZE=4096` |
| `PGSIZE` / `PGSHIFT` | `4096` / `12` | `mmu.h` |
| 断点命中时的 `sp` | **`0x80046eb0`** | OpenSBI 的**固件残留值** |
| `la` 执行后的 `sp` | **`0x80203000`** | 即 `bootstacktop` |
| 设备树 FDT 地址 | `0x87e00000` | `Domain0 Next Arg1` |
| 工具链版本 | gcc **15.1.0** / gdb **16.3.90** / binutils 同版本 | `~/riscv-lab/riscv-elf-toolchains` |
| 模拟器 | QEMU **8.2.2**（apt `qemu-system-misc`） | 自带 OpenSBI **v1.3** |
| 操作系统 | Ubuntu 24.04.5 LTS（WSL2） | |

**能背下来的三行输出**（答辩时可能让你口述）：
```
Domain0 Next Address      : 0x0000000080200000
(THU.CST) os is loading ...
```

**反汇编能背下来的三行**：
```
80200000 <kern_entry>:
  80200000: auipc sp,0x3
  80200004: mv    sp,sp            (= addi sp,sp,0)
  80200008: j     8020000a <kern_init>
```

---

## 十、答辩注意事项

1. **别把"OpenSBI 加载内核"说成事实**。准确说法是：**内核由 QEMU 在机器初始化阶段写入内存**，OpenSBI 只负责初始化硬件并**跳转**过去。（我们实测验证过这一点，说反了容易被追问到答不上。）
2. **`tail` 的关键是 `x0`**，不是"跳转"本身。普通跳转 `call` 也会跳，区别在返回地址存不存。
3. **栈是向低地址增长的**，所以 `sp` 指向 `bootstacktop`（高地址端）才等于空栈。
4. **`sp` 的固件残留值不能用**——这是"为什么要自己立栈"的**实测证据**，能说出来很加分。
5. 被问"你们改过课程代码吗"，如实说：改了 Makefile 两处，都是**环境兼容性修复**，且都在文件里写了注释说明原因；`kern/`、`libs/`、`tools/kernel.ld` **一行没动**。
6. 被问 AI 使用情况，如实说：三人各用不同工具（Claude Code / Cline + DeepSeek），提示词按 lab0.5 四段式框架记录在 `prompt.md`；所有关键数值都是**亲手跑出来的**，报告里还附了反汇编和寄存器实测值供核对。
7. 如果问到 lab1 为什么没有 `make grade`：说明 `grade` 目标虽在 Makefile 里，但它调用的 `tools/grade.sh` 未随 lab1 发放，所以本实验**没有自动评分**，考核依据是练习回答与报告本身。
