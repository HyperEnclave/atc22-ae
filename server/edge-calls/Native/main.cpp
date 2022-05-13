#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

int bench_main()
{
    int warmup_times = 10000;
    int test_times = 1000000;
    latency_t time_rdtscp = bench_in_app("rdtscp", 0, warmup_times, test_times, bench_rdtscp);
    latency_t time_syscall = bench_in_app("syscall", time_rdtscp, warmup_times, test_times, bench_syscall);
    latency_t time_hypercall = bench_in_app("hypercall", time_rdtscp, warmup_times, test_times, bench_hypercall);
    printf("rdtscp: %ld\n", time_rdtscp);
    printf("syscall: %ld\n", time_syscall);
    printf("hypercall: %ld\n", time_hypercall);
    return 0;
}

int main(int argc, char *argv[])
{
    return run_bench("native", bench_main);
}
