//LAB4 function3 2412796

/*
 * switch_to - 保存 from 的上下文，恢复 to 的上下文
 *
 * 参数：a0 = &from->context，a1 = &to->context
 * struct context 依次为 ra, sp, s0..s11，共 14 项，每项 8 字节。
 * 只交换"被调用者保存"寄存器：调用者保存寄存器由编译器在
 * 调用 switch_to 的函数里自动保存/恢复，不必在这里处理。
 *
 * 交换完成后 ret 返回到 to->ra —— 新线程第一次运行时，
 * copy_thread() 已把 context.ra 设成 forkret。
 */
    .text
    .globl switch_to
    .align 2
switch_to:
    sd ra,   0(a0)
    sd sp,   8(a0)
    sd s0,  16(a0)
    sd s1,  24(a0)
    sd s2,  32(a0)
    sd s3,  40(a0)
    sd s4,  48(a0)
    sd s5,  56(a0)
    sd s6,  64(a0)
    sd s7,  72(a0)
    sd s8,  80(a0)
    sd s9,  88(a0)
    sd s10, 96(a0)
    sd s11,104(a0)

    ld ra,   0(a1)
    ld sp,   8(a1)
    ld s0,  16(a1)
    ld s1,  24(a1)
    ld s2,  32(a1)
    ld s3,  40(a1)
    ld s4,  48(a1)
    ld s5,  56(a1)
    ld s6,  64(a1)
    ld s7,  72(a1)
    ld s8,  80(a1)
    ld s9,  88(a1)
    ld s10, 96(a1)
    ld s11,104(a1)

    ret
