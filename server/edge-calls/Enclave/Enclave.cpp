/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

#include "common.h"

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}

void ecall_hello(const char *message)
{
    printf("%s", message);
}

void ecall_empty(void) {}

inline __attribute__((always_inline)) uint64_t enclave_rdtscp()
{
#ifdef ENCLAVE_RDTSC
    return rdtscp();
#else
    uint64_t ret;
    ocall_rdtscp(&ret);
    return ret;
#endif
}

latency_t bench_enclave_rdtscp()
{
    int64_t a = enclave_rdtscp();
    int64_t b = enclave_rdtscp();
    return (latency_t)(b - a);
}

latency_t bench_ocall_empty()
{
    int64_t a = enclave_rdtscp();
    ocall_empty();
    int64_t b = enclave_rdtscp();
    return (latency_t)(b - a);
}

void ecall_bench_enclave_rdtscp(int count, latency_t *all_cycles)
{
    batched_bench(bench_enclave_rdtscp, count, all_cycles);
}

void ecall_bench_ocall_empty(int count, latency_t *all_cycles)
{
    batched_bench(bench_ocall_empty, count, all_cycles);
}

void ecall_get_eenter_stats(long long* count, long long* cycles)
{
#ifdef EENTER_EEXIT_STATS
    extern long long g_eenter_count;
    extern long long g_eenter_cycles;
    if (count && cycles)
    {
        *count = g_eenter_count;
        *cycles = g_eenter_cycles;
    }
#endif
}
