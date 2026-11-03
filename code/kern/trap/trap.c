#include <assert.h>
#include <clock.h>
#include <console.h>
#include <defs.h>
#include <kdebug.h>
#include <memlayout.h>
#include <mmu.h>
#include <riscv.h>
#include <stdio.h>
#include <trap.h>

#define TICK_NUM 100

static void print_ticks() {
    cprintf("%d ticks\n", TICK_NUM);
#ifdef DEBUG_GRADE
    cprintf("End of Test.\n");
    panic("EOT: kernel seems ok.");
#endif
}

/* idt_init - initialize IDT to each of the entry points in kern/trap/vectors.S
 */
void idt_init(void) {

    extern void __alltraps(void);
    /* Set sup0 scratch register to 0, indicating to exception vector
       that we are presently executing in the kernel */
    write_csr(sscratch, 0);
    /* Set the exception vector address */
    write_csr(stvec, &__alltraps);
}

/* trap_in_kernel - test if trap happened in kernel */
bool trap_in_kernel(struct trapframe *tf) {
    return (tf->status & SSTATUS_SPP) != 0;
}

void print_trapframe(struct trapframe *tf) {
    cprintf("trapframe at %p\n", tf);
    print_regs(&tf->gpr);
    cprintf("  status   0x%08x\n", tf->status);
    cprintf("  epc      0x%08x\n", tf->epc);
    cprintf("  badvaddr 0x%08x\n", tf->badvaddr);
    cprintf("  cause    0x%08x\n", tf->cause);
}

void print_regs(struct pushregs *gpr) {
    cprintf("  zero     0x%08x\n", gpr->zero);
    cprintf("  ra       0x%08x\n", gpr->ra);
    cprintf("  sp       0x%08x\n", gpr->sp);
    cprintf("  gp       0x%08x\n", gpr->gp);
    cprintf("  tp       0x%08x\n", gpr->tp);
    cprintf("  t0       0x%08x\n", gpr->t0);
    cprintf("  t1       0x%08x\n", gpr->t1);
    cprintf("  t2       0x%08x\n", gpr->t2);
    cprintf("  s0       0x%08x\n", gpr->s0);
    cprintf("  s1       0x%08x\n", gpr->s1);
    cprintf("  a0       0x%08x\n", gpr->a0);
    cprintf("  a1       0x%08x\n", gpr->a1);
    cprintf("  a2       0x%08x\n", gpr->a2);
    cprintf("  a3       0x%08x\n", gpr->a3);
    cprintf("  a4       0x%08x\n", gpr->a4);
    cprintf("  a5       0x%08x\n", gpr->a5);
    cprintf("  a6       0x%08x\n", gpr->a6);
    cprintf("  a7       0x%08x\n", gpr->a7);
    cprintf("  s2       0x%08x\n", gpr->s2);
    cprintf("  s3       0x%08x\n", gpr->s3);
    cprintf("  s4       0x%08x\n", gpr->s4);
    cprintf("  s5       0x%08x\n", gpr->s5);
    cprintf("  s6       0x%08x\n", gpr->s6);
    cprintf("  s7       0x%08x\n", gpr->s7);
    cprintf("  s8       0x%08x\n", gpr->s8);
    cprintf("  s9       0x%08x\n", gpr->s9);
    cprintf("  s10      0x%08x\n", gpr->s10);
    cprintf("  s11      0x%08x\n", gpr->s11);
    cprintf("  t3       0x%08x\n", gpr->t3);
    cprintf("  t4       0x%08x\n", gpr->t4);
    cprintf("  t5       0x%08x\n", gpr->t5);
    cprintf("  t6       0x%08x\n", gpr->t6);
}

//lab3 function2 2410868

/* interrupt_handler - 处理中断类 Trap
 * scause 的最高位为 1 表示这是一个中断，真正的中断号位于低位；
 * 这里先剥离标志位，得到一个可以直接用于 switch 的中断号。 */
void interrupt_handler(struct trapframe *tf) {
    uintptr_t cause = tf->cause & ~(1UL << 63);

    switch (cause) {
    case IRQ_S_TIMER:
        /* sbi_set_timer() 安排的是一次性时钟事件：本次中断处理完就作废，
         * 所以必须先重新设置下一次，否则时钟中断只会到来一次。 */
        clock_set_next_event();
        ticks++;
        if (ticks % TICK_NUM == 0) {
            print_ticks();
        }
        break;
    case IRQ_S_SOFT:
        cprintf("Supervisor software interrupt\n");
        break;
    case IRQ_S_EXT:
        cprintf("Supervisor external interrupt\n");
        break;
    default:
        /* 未知中断：先输出原因便于检索，再打印完整 Trap frame 兜底。
         * 这里不能 panic()，未识别中断属于可恢复情形。 */
        cprintf("Unknown interrupt: cause %d\n", (int)cause);
        print_trapframe(tf);
        break;
    }
}

/* exception_handler - 处理异常类 Trap
 * 异常是同步事件，cause 为较小的正值，不需要剥离标志位。
 * 关键在于修正 tf->epc：sret 会把 sepc 装回 pc，若不前进，
 * 返回后会再次执行同一条异常指令，形成异常循环。 */
void exception_handler(struct trapframe *tf) {
    switch (tf->cause) {
    case CAUSE_ILLEGAL_INSTRUCTION: {
        cprintf("Illegal instruction at 0x%016lx\n", tf->epc);
        /* 按 RISC-V 指令编码判断长度：最低两位为 0b11 表示 32 位指令，
         * 否则为 16 位压缩指令。不能一律按 +4 处理。 */
        uint16_t insn = *(uint16_t *)tf->epc;
        tf->epc += ((insn & 0x3) == 0x3) ? 4 : 2;
        break;
    }
    case CAUSE_BREAKPOINT:
        cprintf("Breakpoint at 0x%016lx\n", tf->epc);
        /* ebreak 是 4 字节指令 */
        tf->epc += 4;
        break;
    default:
        /* 其他异常本实验不要求具体恢复，打印 Trap frame 后直接返回 */
        cprintf("Unknown exception: cause %d\n", (int)tf->cause);
        print_trapframe(tf);
        break;
    }
}


static inline void trap_dispatch(struct trapframe *tf) {
    if ((intptr_t)tf->cause < 0) {
        // interrupts
        interrupt_handler(tf);
    } else {
        // exceptions
        exception_handler(tf);
    }
}

/* *
 * trap - handles or dispatches an exception/interrupt. if and when trap()
 * returns,
 * the code in kern/trap/trapentry.S restores the old CPU state saved in the
 * trapframe and then uses the iret instruction to return from the exception.
 * */
void trap(struct trapframe *tf) {
    // dispatch based on what type of trap occurred
    trap_dispatch(tf);
}
