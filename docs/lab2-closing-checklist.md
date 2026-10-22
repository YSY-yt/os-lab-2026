# Lab 2 收尾清单

> 本文件列出 lab2 交付前的**全部剩余事项**，按负责人分开，每条都标明**位置、做法、验证方式**。
> 仓库：<https://github.com/YSY-yt/os-lab-2026>　工作分支：**`lab2`**
> 更新日期：2026-10-22

---

## 一、当前状态（已实测验证）

| 检查项 | 状态 | 证据 |
|---|---|---|
| `make grade` 自动评分 | ✅ **20/20** | `check page init: OK` / `check best fit: OK` / `Total Score: 20/20`（截图 `report/images/lab2_grade.png`） |
| Buddy System 自研测试 | ✅ **15/15 通过** | `[buddy] ====== ALL 15 CASES PASSED ======` |
| 报告六节结构 | ✅ 齐全 | 465+ 行，`## 一` ~ `## 六` 完整 |
| 报告占位符 | ✅ **0 处** | 无 `[待填写]` / `[待定]` / `YYYY-MM-DD` |
| `prompt.md` 占位符 | ✅ **0 处** | 四节齐全 |
| 报告图片引用 | ✅ 2 张全部有效 | `lab2_qemu.png`、`lab2_grade.png` |
| `//lab2` 标记 + 学号 | ✅ 两处都在 | `//lab2 function1 2413628`、`//lab2 function2-2 2410868` |
| 分支结构 | ✅ 合规 | 根下只有 `code/` 与 `report/` |

**结论：核心交付已经达标。下面两节是「锦上添花」，不做也不影响交付，但做了报告更完整。**

---

## 二、黄乐仁（部分一）

### 任务 1：把 `entry.S` 的修复写进你自己的章节 🟡 建议

**为什么要写**
你修的启动页表（补低 1G 恒等映射）是全组**最有价值的 bug 修复之一**，但目前只出现在第三节和第六节（组长写的），**你自己的「功能模块一」章节里完全没提**。课程明确要求写「用提示词让 AI 生成代码的过程中出现了什么 bug、怎么解决的」，这段是现成的素材。

**写在哪**
`report/report.md` → 「### 功能模块一：物理内存初始化」→ 「任务一」的「**实现迭代过程**」末尾，追加一段（或新开一小节「启动页表的修复」）。

**写什么（按这个结构）**

1. **现象**：内核跑起来后没有输出（或直接崩溃）——`make qemu` 停在 OpenSBI 之后
2. **定位**：`entry.S` 在 `csrw satp` 之后、跳转到高地址 `kern_init` 之前，PC 仍停留在低地址 `0x80200000` 附近；而原页表 `.zero 8 * 511` 把第 0~510 项全部清零，**只保留了第 511 项（高 1G）的映射**，低地址取指立刻缺页
3. **修复**：把第 2 项（VA `0x80000000` 起 1 GiB）补上低地址恒等映射，使引导代码在开启分页后仍能取指
4. **验证**：修复后 `make grade` 从 0 分变为 **20/20**

> 顺带一提：原文件里那句 `#nop # 可能映射的位置有些bug。。插入一个nop` 就是作者自己留的提示，可以引用。

**验证方式**
```bash
cd ~/os-lab-2026
git pull
# 确认报告里出现 entry.S / 恒等映射 的说明
grep -n "恒等映射" report/report.md
git add -A && git commit -m "报告：补充启动页表缺页问题的定位与修复" && git push
```

### 任务 2：把 AI 工具名写具体 🟢 次要

**位置**：`report/report.md` 第二节的 AI 工具表

| 现在 | 建议改成 |
|---|---|
| `Claude` | `Claude Code（终端 Agent）` 或你实际使用的具体工具名 |

---

## 三、孙锘梵（部分二）

### 任务 1：补 `prompt.md` 的「bug 与解决」记录 🟡 建议

**为什么要补**
`prompt.md` 里部分一、部分三都有「**生成的代码出了什么问题、怎么解决的**」这一块，**唯独部分二没有**，三节格式不一致。

**好消息**：你在 `report/report.md` 第 300–304 行已经写得很完整了，**直接浓缩后搬过去即可**。

**怎么做**

1. 打开 `report/prompt.md`，找到「## 二、部分二：页面分配算法」，定位到最后一节「### 提示词 3：执行修改」的代码块结束处
2. 在代码块之后、`---` 之前，插入：

```markdown
**生成的代码出了什么问题、怎么解决的：**

**第一轮：生成可编译的空壳。** `best_fit_pmm.c` 只有两个测试函数，
`free_list` / `nr_free` 未声明导致编译失败。参照 `default_pmm.c` 补齐
`free_area`、两个宏、五个管理函数与 `best_fit_pmm_manager` 注册结构，
此时分配逻辑仍是 First-Fit，整棵树已能编译。

**第二轮：把分配策略从 First-Fit 改成 Best-Fit。** 两者共用同一套数据结构，
区别只在"分配时选哪个块"。因此只需改 `best_fit_alloc_pages` 的查找循环：
不再"遇到第一个 `property >= n` 就 break"，而是遍历整条空闲链表，
保留 `property` 最小的那个够用块。
```

**验证方式**
```bash
cd ~/os-lab-2026
git pull
grep -c "生成的代码出了什么问题" report/prompt.md   # 应输出 3
git add -A && git commit -m "prompt.md：补充部分二的迭代记录" && git push
```

### 任务 2：迭代表补一行 🟢 可选

**位置**：`report/prompt.md` 末尾「## 四、迭代记录」表格（现 4 行）

**追加**：

| 轮次 | 提示词 | 出现的问题 | 如何修改提示词解决 |
|---|---|---|---|
| 5 | 部分二 · 提示词三（执行修改） | 空壳阶段按 First-Fit 实现，与任务要求的 Best-Fit 不符 | 在 `[SPECIFICATION]` 中明确"遍历整条链表，选 `property >= n` 且 `property` 最小者"，只改查找循环、不动切分与释放逻辑 |

---

## 四、杨斯源（组长）

### 任务 1：终检 ⏳

等两位组员改完并推送后，在本地执行：

```bash
cd /d/os-lab-2026
git checkout lab2
git pull

# 1) 代码仍能编译、评分仍是 20/20
cd code && make clean && make grade        # 期望 Total Score: 20/20
cd ..

# 2) 报告与提示词无占位符
grep -cE '\[待填写\]|\[待定\]|YYYY-MM-DD|待本人确认' report/report.md   # 期望 0
grep -c '\[待填写\]' report/prompt.md                                  # 期望 0

# 3) 三节格式一致（期望 3）
grep -c '生成的代码出了什么问题' report/prompt.md

# 4) 图片引用有效
grep -o '\./images/[a-zA-Z0-9_.]*' report/report.md | sort -u
ls report/images/

# 5) 分支结构
git ls-tree --name-only HEAD          # 期望 .gitattributes .gitignore README.md code report
```

### 任务 2：状态更新 ⏳

| 文件 | 改什么 |
|---|---|
| `README.md`（`main`） | 实验进度表：`lab2` → **✅ 已完成** |
| `README.md`（`lab2` 分支） | 同上 |
| `docs/lab2-tasks.md`（`main`） | 顶部加一行「**本实验已完成，`make grade` 20/20**」 |

### 任务 3：最终推送 ⏳

```bash
cd /d/os-lab-2026
git checkout lab2 && git push origin lab2
git checkout main && git push origin main
```

---

## 五、最终验收清单（交付前逐条打勾）

### 代码

- [ ] `cd code && make clean && make` 在 `-Werror` 下无警告通过
- [ ] `make qemu` 输出 `memory management: best_fit_pmm_manager` 与 `check_alloc_page() succeeded!`
- [ ] `make grade` 为 **20/20**
- [ ] `//lab2 function1 2413628`、`//lab2 function2-2 2410868` 标记均保留
- [ ] `bin/`、`obj/` 未被提交

### 报告

- [ ] 六节齐全，**全文无占位符**
- [ ] 三处「**负责人：**」均已署名
- [ ] 「小组分工」表 3 行已填实
- [ ] 完成日期已填
- [ ] AI 工具表 3 行均已填
- [ ] 两道思考题（First-Fit / Best-Fit 改进空间）均已作答
- [ ] Challenge 思考题（可用物理内存范围）已作答
- [ ] Buddy 的「测试覆盖说明」已作答
- [ ] 每个任务都有「最终提示词」**和**「实现迭代过程」，**没有空标题**

### 提示词

- [ ] `prompt.md` 三节格式一致（三处都有「生成的代码出了什么问题」）
- [ ] 迭代记录表已填写

### 图片与分支

- [ ] `report/images/` 下有 `lab2_qemu.png` 与 `lab2_grade.png`
- [ ] 报告中的图片链接在 GitHub 网页上能正常显示
- [ ] 所有成果在 `lab2` 分支，未提交到 `main`
- [ ] `lab2` 根目录下正好是 `code/` 与 `report/`

---

## 六、给所有人的三条提醒

1. **每次提交前先 `git pull`** —— 分支上已有三个人的提交，不拉取会被拒（需要变基）。
2. **不要改 `tools/test_lab2.c`、`tools/inject.py`、`tools/grade.sh` 的评分逻辑** —— 评分时会把这些文件注入到临时副本，改了也不生效，反而可能被判违规。
3. **不要改 `best_fit_pmm.c` 里的 `basic_check` / `best_fit_check` 两个测试函数** —— 课程明确禁止。

---

## 七、答辩可能被问到的问题（预告）

| 问题 | 答题要点 |
|---|---|
| 为什么内核要搬到高虚拟地址？ | 地址空间布局与内核/用户隔离；链接时就固化了地址 |
| `entry.S` 里为什么需要低 1G 恒等映射？ | `csrw satp` 后、跳高地址前，PC 仍在低地址，没有映射就取指缺页 |
| `page_init` 为什么要保留描述符数组自身？ | 否则数组所在页会被当空闲页分配出去，描述符被破坏 |
| Best-Fit 和 First-Fit 的区别？改进空间？ | 前者遍历全表取最小够用块；仍 O(n)，可用按大小排序结构/分离适配改进 |
| Buddy 的伙伴地址怎么算？ | `index ^ (1 << order)`，且块必须对齐到 `2^order` |
| Buddy 测试怎么验证合并正确？ | 三条不变量：对齐、无未合并同阶伙伴、页数守恒 |
