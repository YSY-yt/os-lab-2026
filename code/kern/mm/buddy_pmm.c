#include <buddy_pmm.h>
#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <pmm.h>
#include <stdio.h>

/*
 * Buddy System（伙伴系统）连续物理内存分配器
 *
 * 约定：
 *   - 每个阶（order）一条空闲链表 free_area[k]，存放 2^k 页空闲块的头页；
 *   - 空闲块头页置 PG_property，property = order；
 *   - 已分配块清 PG_property，但 property 保留该块的 order，供 free_pages 还原大小；
 *   - nr_free 以"页"为单位（不是块数）；
 *   - 块头页在 pages 数组中的下标必须对齐到 2^order，这是伙伴地址计算正确的前提。
 *
 * 容量：可管理物理内存 128 MiB ÷ 4 KiB = 32768 页 = 2^15，
 *       故最大块为 order 15，MAX_ORDER 取 16（合法 order 为 0..15）。
 */
#define MAX_ORDER   16
#define ORDER_PAGES(k)  (1u << (k))

static free_area_t free_area[MAX_ORDER];
static unsigned int nr_free = 0;

/* pages 数组下标（相对被管理区域起点） */
static inline size_t
page_index(struct Page *p) {
    return (size_t)(p - pages);
}

/* 被管理区域的总页数 */
static inline size_t
managed_pages(void) {
    return (size_t)(npage - nbase);
}

/* 求 ceil(log2(n))；n == 0 或超出 MAX_ORDER 时返回 -1 */
static int
pages_order(size_t n) {
    int k = 0;
    size_t cap = 1;
    if (n == 0) {
        return -1;
    }
    while (cap < n) {
        cap <<= 1;
        k++;
    }
    return (k < MAX_ORDER) ? k : -1;
}

/* 把 order 为 k、头页为 base 的块插入空闲链表 */
static void
add_free_block(struct Page *base, int k) {
    base->property = (unsigned int)k;
    SetPageProperty(base);
    list_add(&free_area[k].free_list, &(base->page_link));
    free_area[k].nr_free += ORDER_PAGES(k);
    nr_free += ORDER_PAGES(k);
}

static void
buddy_init(void) {
    for (int i = 0; i < MAX_ORDER; i++) {
        list_init(&free_area[i].free_list);
        free_area[i].nr_free = 0;
    }
    nr_free = 0;
}

/*
 * 把 [base, base+n) 登记为可分配。
 * 区域起点不一定按 2 的幂对齐、长度也不一定是 2 的幂，
 * 因此必须拆成若干"起点对齐、大小为 2^k"的块逐块登记。
 */
static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);

    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }

    size_t idx  = page_index(base);
    size_t left = n;
    while (left > 0) {
        int k = 0;
        while (k + 1 < MAX_ORDER
               && ORDER_PAGES(k + 1) <= left
               && (idx & (ORDER_PAGES(k + 1) - 1)) == 0) {
            k++;
        }
        add_free_block(base, k);
        base += ORDER_PAGES(k);
        idx  += ORDER_PAGES(k);
        left -= ORDER_PAGES(k);
    }
}

static struct Page *
buddy_alloc_pages(size_t n) {
    if (n == 0) {
        return NULL;
    }
    int k = pages_order(n);
    if (k < 0) {
        return NULL;
    }

    /* 找到第一个非空且阶 >= k 的空闲链表 */
    int m = k;
    while (m < MAX_ORDER && list_empty(&free_area[m].free_list)) {
        m++;
    }
    if (m >= MAX_ORDER) {
        return NULL;
    }

    /* 从 m 逐级对半分裂到 k：每次拆成两个 order-1 的块并插回该阶链表，
     * 其中一个继续参与下一轮分裂。同阶块彼此等价，取链表首元素即可。 */
    while (m > k) {
        list_entry_t *le = list_next(&free_area[m].free_list);
        struct Page *blk = le2page(le, page_link);

        list_del(&(blk->page_link));
        ClearPageProperty(blk);
        free_area[m].nr_free -= ORDER_PAGES(m);
        nr_free -= ORDER_PAGES(m);

        int half = m - 1;
        add_free_block(blk + ORDER_PAGES(half), half);   /* 右半 */
        add_free_block(blk, half);                       /* 左半，继续分裂 */
        m = half;
    }

    /* 从 k 阶链表取一块分配出去 */
    list_entry_t *le = list_next(&free_area[k].free_list);
    struct Page *blk = le2page(le, page_link);
    list_del(&(blk->page_link));
    ClearPageProperty(blk);
    free_area[k].nr_free -= ORDER_PAGES(k);
    nr_free -= ORDER_PAGES(k);

    blk->property = (unsigned int)k;    /* 记录 order，供 free_pages 使用 */
    return blk;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    assert(base != NULL);

    /* 块大小必须从 property 取，不能用参数 n 反推 */
    int k = (int)(base->property);
    assert(k >= 0 && k < MAX_ORDER);

    /* 尝试与伙伴合并，可递归向上 */
    while (k < MAX_ORDER - 1) {
        size_t bidx = page_index(base) ^ ORDER_PAGES(k);
        if (bidx >= managed_pages()) {
            break;                       /* 伙伴越界，无法合并 */
        }
        struct Page *buddy = pages + bidx;
        if (!PageProperty(buddy) || buddy->property != (unsigned int)k) {
            break;                       /* 伙伴不空闲或不同阶 */
        }

        /* 从该阶链表摘除伙伴；全局 nr_free 也要减去它，
         * 否则最后按合并后的整块大小累加时会重复计算伙伴的页数。 */
        list_del(&(buddy->page_link));
        ClearPageProperty(buddy);
        free_area[k].nr_free -= ORDER_PAGES(k);
        nr_free -= ORDER_PAGES(k);

        /* 合并后的头页取地址较小者 */
        if (buddy < base) {
            base = buddy;
        }
        k++;
    }

    add_free_block(base, k);
}

static size_t
buddy_nr_free_pages(void) {
    return (size_t)nr_free;
}

/* ======================== 自研测试 ======================== */

#define DRAIN_MAX 128

/* 全局不变量检查：
 *   1) 每个头页 PageProperty 为真且 property == 所在阶
 *   2) 块头页下标必须对齐到 2^order
 *   3) 不存在"应当合并却未合并"的同阶相邻伙伴
 *   4) 各阶空闲页数之和 == 全局 nr_free == nr_free_pages()
 */
static void
buddy_check_invariants(void) {
    unsigned int sum = 0;

    for (int k = 0; k < MAX_ORDER; k++) {
        unsigned int pgs = 0;
        list_entry_t *le = &free_area[k].free_list;
        while ((le = list_next(le)) != &free_area[k].free_list) {
            struct Page *p = le2page(le, page_link);
            assert(PageProperty(p));
            assert((int)p->property == k);
            assert((page_index(p) & (ORDER_PAGES(k) - 1)) == 0);

            if (k < MAX_ORDER - 1) {
                size_t bi = page_index(p) ^ ORDER_PAGES(k);
                if (bi < managed_pages()) {
                    struct Page *buddy = pages + bi;
                    assert(!(PageProperty(buddy) &&
                             buddy->property == (unsigned int)k));
                }
            }
            pgs += ORDER_PAGES(k);
        }
        assert(pgs == free_area[k].nr_free);
        sum += free_area[k].nr_free;
    }
    assert(sum == nr_free);
    assert(sum == buddy_nr_free_pages());
}

static void
buddy_check(void) {
    struct Page *p, *a, *b;
    size_t base_free, before;
    int i, n;

    cprintf("[buddy] ====== Buddy System self-test begin ======\n");

    /* Case 1：初始一致性 */
    buddy_check_invariants();
    base_free = nr_free_pages();
    assert(base_free > 0);
    cprintf("[buddy] Case 1  initial consistency ......... OK (%lu pages)\n",
            (unsigned long)base_free);

    /* Case 2：单页分配 / 释放 */
    before = nr_free_pages();
    p = alloc_page();
    assert(p != NULL);
    assert(nr_free_pages() == before - 1);
    free_page(p);
    assert(nr_free_pages() == before);
    buddy_check_invariants();
    cprintf("[buddy] Case 2  alloc/free 1 page ........... OK\n");

    /* Case 3：2 的幂分配（2/4/8 页） */
    for (i = 2; i <= 8; i <<= 1) {
        before = nr_free_pages();
        p = alloc_pages((size_t)i);
        assert(p != NULL);
        assert(nr_free_pages() == before - (size_t)i);
        free_pages(p, (size_t)i);
        assert(nr_free_pages() == before);
    }
    buddy_check_invariants();
    cprintf("[buddy] Case 3  alloc/free 2/4/8 pages ...... OK\n");

    /* Case 4：非 2 的幂请求（申请 3 页，实际占用 4 页） */
    before = nr_free_pages();
    p = alloc_pages(3);
    assert(p != NULL);
    assert(nr_free_pages() == before - 4);
    free_pages(p, 3);
    assert(nr_free_pages() == before);
    buddy_check_invariants();
    cprintf("[buddy] Case 4  request 3 -> occupies 4 ...... OK\n");

    /* Case 5：大块的分裂与合并（4 MiB） */
    before = nr_free_pages();
    p = alloc_pages(1024);
    assert(p != NULL);
    assert(nr_free_pages() == before - 1024);
    free_pages(p, 1024);
    assert(nr_free_pages() == before);
    buddy_check_invariants();
    cprintf("[buddy] Case 5  split/coalesce 4 MiB ........ OK\n");

    /* Case 6：只释放一块（伙伴仍被占用）不得合并 */
    a = alloc_pages(4);
    b = alloc_pages(4);
    assert(a != NULL && b != NULL && a != b);
    before = nr_free_pages();
    free_pages(a, 4);
    assert(nr_free_pages() == before + 4);
    buddy_check_invariants();
    cprintf("[buddy] Case 6  free one of a pair .......... OK\n");

    /* Case 7：释放另一块后应发生合并 */
    free_pages(b, 4);
    buddy_check_invariants();
    cprintf("[buddy] Case 7  free both -> coalesce ....... OK\n");

    /* Case 8：8 页连续分配后全部释放，应逐级合并 */
    before = nr_free_pages();
    p = alloc_pages(8);
    assert(p != NULL);
    assert(nr_free_pages() == before - 8);
    free_pages(p, 8);
    assert(nr_free_pages() == before);
    buddy_check_invariants();
    cprintf("[buddy] Case 8  recursive coalesce .......... OK\n");

    /* Case 9：大块耗尽后返回 NULL，全部释放后可恢复 */
    {
        static struct Page *drained[DRAIN_MAX];
        int cnt = 0;
        before = nr_free_pages();
        while (cnt < DRAIN_MAX) {
            struct Page *q = alloc_pages(1024);
            if (q == NULL) {
                break;
            }
            drained[cnt++] = q;
        }
        assert(cnt < DRAIN_MAX);        /* 必须真的耗尽到分配失败 */
        assert(nr_free_pages() < 1024); /* 已不足一个 1024 页的块 */
        for (i = cnt - 1; i >= 0; i--) {
            free_pages(drained[i], 1024);
        }
        assert(nr_free_pages() == before);
        buddy_check_invariants();
        cprintf("[buddy] Case 9  exhaust (%d chunks) then free . OK\n", cnt);
    }

    /* Case 10：反复分配释放不泄漏 */
    for (n = 0; n < 3; n++) {
        p = alloc_pages(16);
        assert(p != NULL);
        free_pages(p, 16);
        assert(nr_free_pages() == base_free);
    }
    cprintf("[buddy] Case 10 repeated alloc/free ......... OK\n");

    /* Case 11：alloc_pages(0) 返回 NULL */
    assert(alloc_pages(0) == NULL);
    cprintf("[buddy] Case 11 alloc_pages(0) -> NULL ...... OK\n");

    /* Case 12/13：超大请求返回 NULL */
    assert(alloc_pages((size_t)ORDER_PAGES(MAX_ORDER - 1) * 2) == NULL);
    assert(alloc_pages(100000) == NULL);
    cprintf("[buddy] Case 12/13 oversize request -> NULL . OK\n");

    /* Case 14：释放后立即按同大小再分配 */
    a = alloc_pages(8);
    assert(a != NULL);
    free_pages(a, 8);
    b = alloc_pages(8);
    assert(b != NULL);
    free_pages(b, 8);
    buddy_check_invariants();
    cprintf("[buddy] Case 14 realloc after free .......... OK\n");

    /* Case 15：两种释放顺序结果一致 */
    a = alloc_pages(4);
    b = alloc_pages(4);
    assert(a != NULL && b != NULL);
    if (a > b) { struct Page *t = a; a = b; b = t; }
    free_pages(a, 4);        /* 先低地址 */
    free_pages(b, 4);
    assert(nr_free_pages() == base_free);
    a = alloc_pages(4);
    b = alloc_pages(4);
    assert(a != NULL && b != NULL);
    if (a > b) { struct Page *t = a; a = b; b = t; }
    free_pages(b, 4);        /* 先高地址 */
    free_pages(a, 4);
    assert(nr_free_pages() == base_free);
    buddy_check_invariants();
    cprintf("[buddy] Case 15 both free orders ........... OK\n");

    assert(nr_free_pages() == base_free);
    cprintf("[buddy] ====== ALL 15 CASES PASSED ======\n");
}

const struct pmm_manager buddy_pmm_manager = {
    .name          = "buddy_pmm_manager",
    .init          = buddy_init,
    .init_memmap   = buddy_init_memmap,
    .alloc_pages   = buddy_alloc_pages,
    .free_pages    = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check         = buddy_check,
};
