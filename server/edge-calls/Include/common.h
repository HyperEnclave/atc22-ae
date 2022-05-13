#ifndef _COMMON_H
#define _COMMON_H

#include "user_types.h"

inline __attribute__((always_inline)) uint64_t rdtscp()
{
    register uint64_t lo, hi;
    __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    return ((uint64_t)hi << 32) | lo;
}

latency_t bench_rdtscp()
{
    uint64_t a = rdtscp();
    uint64_t b = rdtscp();
    return (latency_t)(b - a);
}

latency_t bench_syscall()
{
    uint64_t a = rdtscp();
    __asm__ volatile("syscall"::"a"(0x8000dead));
    uint64_t b = rdtscp();
    return b - a;
}

latency_t bench_hypercall()
{
    uint64_t a = rdtscp();
    __asm__ volatile("vmmcall"::"a"(0x8000dead));
    uint64_t b = rdtscp();
    return b - a;
}

void batched_bench(latency_t (*bench_fn)(), int count, latency_t *all_cycles)
{
    bench_fn(); // warm up

    for (int i = 0; i < count; i++)
        all_cycles[i] = bench_fn();
}

#endif // _COMMON_H
