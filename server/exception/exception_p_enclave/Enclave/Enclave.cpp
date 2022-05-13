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

#include "signal.h"
#include "paging.h"

static int g_intr_count = 0;
static int g_pf_count = 0;
static int g_ud_count = 0;

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

    init_exception();
    sgx_register_fast_exception_handler((sgx_fast_exception_handler_t)exception_handler);
    printf_ocall("Register exception handler OK!\n");

    // __asm__ volatile ("sti");
    uint64_t cr3 = read_cr3();
    cache_cr3(cr3);
    printf_ocall("CR3 = 0x%llx\n", cr3);
    printf_ocall("*CR3 = 0x%llx\n", *(uint64_t*)phys_to_virt(cr3));
    // dump_pgtable(cr3, 10);

    for (int i = 0; i < BUF_SIZE; i += PGSIZE)
        test_buf[i] = 0;

    int ret = mprotect(test_buf, BUF_SIZE, PTE_P); // remove write permission
    if (ret != 0) {
        printf_ocall("ERROR: mprotect returns: %d\n", ret);
        abort();
    }
    printf_ocall("read OK: %d\n", test_buf[0]);

    uint64_t time_begin = rdtscp();
    for (int i = 0; i < BUF_SIZE; i += PGSIZE)
        test_buf[i] = 0;
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

    printf_ocall("Interrupt Count = %d\n", g_intr_count);
    printf_ocall("#UD Count = %d\n", g_ud_count);
    printf_ocall("#PF Count = %d\n\n", g_pf_count);

    return SGX_SUCCESS;
}

void dump_trapframe(trapframe_t* tf)
{
    printf_ocall("RAX=%016lx RBX=%016lx RCX=%016lx RDX=%016lx\n", tf->rax, tf->rbx, tf->rcx, tf->rdx);
    printf_ocall("RSI=%016lx RDI=%016lx RBP=%016lx RSP=%016lx\n", tf->rsi, tf->rdi, tf->rbp, tf->rsp);
    printf_ocall("R8 =%016lx R9 =%016lx R10=%016lx R11=%016lx\n", tf->r8, tf->r9, tf->r10, tf->r11);
    printf_ocall("R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n", tf->r12, tf->r13, tf->r14, tf->r15);
    printf_ocall("RIP=%016lx RFL=%016lx CS=%04lx SS=%04lx\n", tf->rip, tf->rflags, tf->cs, tf->ss);
    printf_ocall("vector=%08lx err=%08lx\n\n", tf->vector, tf->err_code);
}

void exception_handler(trapframe_t *info)
{
    // if (g_intr_count <= 10) {
    //     printf_ocall("exception: %lld %llx\n", info->vector, info->err_code);
    // }
    int vec = (int)info->vector;
    switch (vec)
    {
    case 0x6: {
        g_ud_count++;
        info->rip += 2;
        break;
    }
    case 0xE: {
        g_pf_count++;
        uint64_t fault_vaddr = read_cr2();
        // printf_ocall("page fault @ %016llx, err_code=%x\n", fault_vaddr, info->err_code);
        int ret = mprotect((void*)fault_vaddr, PGSIZE, PTE_P | PTE_W);
        if (ret != 0) {
            printf_ocall("ERROR: page fault mprotect returns: %d\n", ret);
            dump_trapframe(info);
            abort();
        }
        break;
    }
    default:
        if (vec >= 32) {
            // tell RustMonitor to inject the interrupt to primay OS
            g_intr_count++;
            int ret;
            __asm__ volatile("vmmcall" : "=a"(ret) : "a"(0x80002333), "D"(vec), "S"(info->err_code));
        } else {
            printf_ocall("ERROR: unexcept exception: %lld %llx\n", vec, info->err_code);
            dump_trapframe(info);
            abort();
        }
        break;
    }
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
