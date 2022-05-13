#ifndef _MM_H_
#define _MM_H_

#include <stdint.h>

#define FLAG_R  (1 << 0)
#define FLAG_W  (1 << 1)
#define FLAG_X  (1 << 2)
#define FLAG_U  (1 << 9)

static int mprotect(void *addr, size_t len, uint64_t flags)
{
    int ret;
    __asm__ volatile("vmmcall" : "=a"(ret) : "a"(0x80006666), "D"(addr), "S"(len), "c"(flags));
    return ret;
}

#endif // _MM_H_
