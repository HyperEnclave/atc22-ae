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

#include <stdarg.h>
#include <stdio.h>
#include "Enclave.h"
#include "Enclave_t.h"

#include "mm.h"

static int g_ud_count = 0;

const int PGSIZE = 4096;
const int BUF_SIZE = 128 << 20;

uint8_t test_buf[BUF_SIZE] __attribute__((aligned (4096)));

inline __attribute__((always_inline)) uint64_t rdtscp()
{
    register uint64_t lo, hi;
    __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");
    return ((uint64_t)hi << 32) | lo;
}

sgx_status_t ecall_exception(uint32_t nrepeats) {
    printf_ocall("Starting...\n");

    void * handler = sgx_register_exception_handler(true, (sgx_exception_handler_t)exception_handler);
    if (handler == NULL) {
        return SGX_ERROR_UNEXPECTED;
    }
    printf_ocall("Register exception handler OK!\n");

    for (int i = 0; i < BUF_SIZE; i += PGSIZE)
        test_buf[i] = 0;

    int ret = mprotect(test_buf, BUF_SIZE, FLAG_R | FLAG_U); // remove write permission
    if (ret != 0) {
        printf_ocall("ERROR: mprotect returns: %d\n", ret);
        abort();
    }
    printf_ocall("read OK: %d\n", test_buf[0]);

    uint64_t time_begin = rdtscp();
    for (int i = 0; i < BUF_SIZE; i += PGSIZE)
        test_buf[i] = 1;
    uint64_t time_end = rdtscp();
    uint64_t total_time = time_end - time_begin - 60;
    printf_ocall("write OK\n");
    printf_ocall("Write Count = %d\n", BUF_SIZE / PGSIZE);
    printf_ocall("Total Time = %lld (cycles)\n", total_time);
    printf_ocall("Average time = %.3lf (cycles)\n\n", (double)total_time / (BUF_SIZE / PGSIZE));
    // test_buf[0] = 233;
    // printf_ocall("write OK\n");
    // printf_ocall("access buf OK!\n\n");

    time_begin = rdtscp();
    for (uint32_t i = 0; i < nrepeats; i ++) {
        __asm(
            "ud2"
        );
    }
    time_end = rdtscp();
    total_time =  time_end - time_begin - 60;
    printf_ocall("test #UD OK\n");
    printf_ocall("Count = %d\n", nrepeats);
    printf_ocall("Total Time = %lld (cycles)\n", total_time);
    printf_ocall("Average time = %.3lf (cycles)\n\n", (double)total_time / nrepeats);

    printf_ocall("#UD Count = %d\n", g_ud_count);

    sgx_unregister_exception_handler(handler);
    return SGX_SUCCESS;
}

int exception_handler(sgx_exception_info_t *info)
{
    int vec = info->exception_vector;
    switch (vec)
    {
    case 0x6: {
        g_ud_count++;
        info->cpu_context.rip += 2;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    default:
        printf_ocall("ERROR: unexcept exception: %lld @ %016llx\n", vec, info->cpu_context.rip);
        break;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

void printf_ocall(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}
