#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <stdint.h>

#define CS_SEL (1 << 3)

struct trapframe_t {
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t vector;
    uint64_t err_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;

    uint64_t rsp;
    uint64_t ss;
};

struct gatedesc {
    uint32_t off_15_0 : 16;   // low 16 bits of offset in segment
    uint32_t cs : 16;         // code segment selector
    uint32_t ist : 3;         // interrupt-stack-table
    uint32_t _rsv0 : 5;       // reserved (ignored)
    uint32_t type : 4;        // type
    uint32_t s : 1;           // must be 0 (system)
    uint32_t dpl : 2;         // descriptor privilege level
    uint32_t p : 1;           // Present
    uint32_t off_31_16 : 16;  // high bits of offset in segment
    uint32_t off_63_32 : 32;  // high bits of offset in segment
    uint32_t _rsv1 : 32;      // reserved (ignored)
};

struct gatedesc idt[256];

#define SETGATE(gate, sel, off)                     \
    {                                               \
        (gate).off_15_0 = (uint32_t)(off)&0xffff;   \
        (gate).cs = (sel);                          \
        (gate).ist = 0;                             \
        (gate)._rsv0 = 0;                           \
        (gate).type = 0b1110;                       \
        (gate).s = 0;                               \
        (gate).dpl = 0;                             \
        (gate).p = 1;                               \
        (gate).off_31_16 = (uint16_t)((off) >> 16); \
        (gate).off_63_32 = (uint32_t)((off) >> 32); \
        (gate)._rsv1 = 0;                           \
    }

static inline void lidt(struct gatedesc* p, size_t size)
{
    volatile uint16_t pd[5];
    pd[0] = (uint16_t)(size - 1);
    pd[1] = (uint16_t)(uintptr_t)p;
    pd[2] = (uint16_t)((uintptr_t)p >> 16);
    pd[3] = (uint16_t)((uintptr_t)p >> 32);
    pd[4] = (uint16_t)((uintptr_t)p >> 48);
    __asm__ volatile("lidt (%0)" : : "r"(pd) : "memory");
}

typedef void (*sgx_fast_exception_handler_t)(trapframe_t *info);

sgx_fast_exception_handler_t g_handler;

void sgx_register_fast_exception_handler(sgx_fast_exception_handler_t exception_handler) {
    g_handler = exception_handler;
}

void init_exception()
{
    extern long long exception_entries[256];
    for (int i = 0; i < 256; i++)
        SETGATE(idt[i], CS_SEL, exception_entries[i]);
    lidt(idt, sizeof(idt));
}

extern "C" void common_exception_handler(trapframe_t* tf)
{
    if (g_handler != NULL)
        g_handler(tf);
}

#endif // _SIGNAL_H_
