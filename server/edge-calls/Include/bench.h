#ifndef _BENCH_H_
#define _BENCH_H_

#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"

#define MAX_RUN_TIMES 2000000

static FILE* g_stats_file;

int cmpfunc(const void* a, const void* b)
{
    const latency_t aa = *(const latency_t*)a;
    const latency_t bb = *(const latency_t*)b;
    return aa > bb ? 1 : aa < bb ? -1 : 0;
}

latency_t bench_stats(const char* name, latency_t time_offset, int warmup_times, int test_times,
                        const latency_t* all_cycles) {
    int n = test_times;
    static latency_t t[MAX_RUN_TIMES];

    for (int i = 0; i < n; i++)
        t[i] = all_cycles[i + warmup_times];
    qsort(t, n, sizeof(t[0]), cmpfunc);

    double sum = 0, sum2 = 0, min = t[0], max = t[n - 1];
    for (int i = 0; i < n; i++)
    {
        sum += t[i];
        sum2 += 1.0 * t[i] * t[i];
    }
    double ave = 1.0 * sum / n;
    double std = sqrt(sum2 / n - ave * ave);
    double median = (t[n / 2] + t[(n - 1) / 2]) / 2.0;
    min -= time_offset;
    max -= time_offset;
    ave -= time_offset;
    median -= time_offset;

    printf("Median = %.3lf\n", median);
    printf("Average = %.3lf\n", ave);
    printf("Min = %.3lf\n", min);
    printf("Max = %.3lf\n", max);
    printf("Deviation = %.3lf\n", std);
    puts("");

    char out_name[256];
    snprintf(out_name, 250, "%s.txt", name);
    FILE *fout = fopen(out_name, "w");
    for (int i = 0; i < n; i++)
        fprintf(fout, "%ld\n", all_cycles[i + warmup_times]);
    fclose(fout);

    fprintf(g_stats_file, "%s,%lf,%lf,%lf,%lf,%lf\n", name, median, ave, min, max, std);

    return (latency_t)(median + 0.5);
}


latency_t bench_in_app(const char* name, latency_t time_offset, int warmup_times, int test_times, latency_t (*bench_fn)())
{
    static latency_t all_cycles[MAX_RUN_TIMES];
    printf("Benchmark %s in App:\n", name);
    batched_bench(bench_fn, warmup_times + test_times, all_cycles);
    return bench_stats(name, time_offset, warmup_times, test_times, all_cycles);
}

#ifdef SGX
#include "App.h"
latency_t bench_in_enclave(const char* name, latency_t time_offset, int warmup_times, int test_times,
                           sgx_status_t (*ecall_bench_fn)(sgx_enclave_id_t, int, latency_t*))
{
    static latency_t all_cycles[MAX_RUN_TIMES];
    printf("Benchmark %s in Enclave:\n", name);
    sgx_status_t ret = ecall_bench_fn(global_eid, warmup_times + test_times, all_cycles);
    if (ret) {
        print_error_message(ret);
        exit(0);
    }
    return bench_stats(name, time_offset, warmup_times, test_times, all_cycles);
}
#endif

int run_bench(const char* tag, int (*bench_main)())
{
    time_t now = time(NULL);

    char result_path[256];
    snprintf(result_path, 250, "result-%s-%ld", tag, now);
    mkdir(result_path, 0775);
    if (chdir(result_path) == -1) return -1;

    g_stats_file = fopen("stats.csv", "w");
    if (!g_stats_file) return -1;
    fprintf(g_stats_file, "Name,Median,Average,Min,Max,Deviation\n");

    int ret = bench_main();

    fclose(g_stats_file);
    return ret;
}

#endif // _BENCH_H_
