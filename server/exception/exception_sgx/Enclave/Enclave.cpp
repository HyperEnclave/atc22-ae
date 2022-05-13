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

static int g_ud_count = 0;

sgx_status_t ecall_exception(uint32_t nrepeats) {
    void * handler = sgx_register_exception_handler(true, (sgx_exception_handler_t)exception_handler);
    if (handler == NULL) {
        return SGX_ERROR_UNEXPECTED;
    }

    uint64_t time_begin, time_end;
    ocall_rdtscp(&time_begin);
    for (uint32_t i = 0; i < nrepeats; i ++) {
        __asm(
            "ud2"
        );
    }
    ocall_rdtscp(&time_end);
    uint64_t total_time =  time_end - time_begin - 12432;
    printf_ocall("test #UD OK\n");
    printf_ocall("Count = %d\n", nrepeats);
    printf_ocall("Total Time = %lld (cycles)\n", total_time);
    printf_ocall("Average time = %.3lf (cycles)\n\n", (double)total_time / nrepeats);

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
