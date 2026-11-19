#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <assert.h>

void
wakeup_proc(struct proc_struct *proc) {
    assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
    proc->state = PROC_RUNNABLE;
}

//LAB4 function3 2412796

/* schedule - 选一个就绪线程并切换过去
 *
 * 选线程有三种情形（对应 ucore 的 FIFO/轮转语义）：
 *   1. 当前不是 idleproc：从"自己的下一个"开始找，实现轮转；
 *   2. 当前就是 idleproc：从链表头开始找，给所有线程平等机会；
 *   3. 一圈都没找到就绪线程：退回 idleproc 保底。
 *
 * 整个过程（清 need_resched、遍历链表、proc_run）必须在关中断的
 * 临界区里完成：否则 current、satp 与实际执行现场可能不一致。
 */
void
schedule(void) {
    bool intr_flag;
    list_entry_t *le, *last;
    struct proc_struct *next = NULL;

    local_intr_save(intr_flag);
    {
        current->need_resched = 0;

        last = (current == idleproc) ? &proc_list : &(current->list_link);
        le = last;
        do {
            if ((le = list_next(le)) != &proc_list) {
                next = le2proc(le, list_link);
                if (next->state == PROC_RUNNABLE) {
                    break;
                }
            }
        } while (le != last);

        if (next == NULL || next->state != PROC_RUNNABLE) {
            next = idleproc;
        }

        next->runs++;
        if (next != current) {
            proc_run(next);
        }
    }
    local_intr_restore(intr_flag);
}


