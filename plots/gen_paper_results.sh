#!/bin/bash

res_dir=../paper-results

python3 fig8a_nbench.py \
    -ab $res_dir/nbench/result-amd-sim.res \
    -ah $res_dir/nbench/result-amd-hyper.res \
    -ib $res_dir/nbench/result-intel-sim.res \
    -is $res_dir/nbench/result-intel-sgx.res

python3 fig8b_sqlite.py \
    -ab $res_dir/sqlite/result-amd-sim.throughput.csv \
    -ag $res_dir/sqlite/result-amd-gu.throughput.csv \
    -ah $res_dir/sqlite/result-amd-hu.throughput.csv \
    -ib $res_dir/sqlite/result-intel-sim.throughput.csv \
    -is $res_dir/sqlite/result-intel-sgx.throughput.csv \
    -m $res_dir/sqlite/memory-usage.csv

python3 fig8c_lighttpd.py \
    -ab $res_dir/lighttpd/result-amd-sim.throughput.csv \
    -ag $res_dir/lighttpd/result-amd-gu.throughput.csv \
    -ah $res_dir/lighttpd/result-amd-hu.throughput.csv \
    -ib $res_dir/lighttpd/result-intel-sim.throughput.csv \
    -is $res_dir/lighttpd/result-intel-sgx.throughput.csv

python3 fig8d_redis.py \
    -ab $res_dir/redis/result-amd-sim.throughput.csv $res_dir/redis/result-amd-sim.latency.csv \
    -ag $res_dir/redis/result-amd-gu.throughput.csv $res_dir/redis/result-amd-gu.latency.csv \
    -ah $res_dir/redis/result-amd-hu.throughput.csv $res_dir/redis/result-amd-hu.latency.csv \
    -ib $res_dir/redis/result-intel-sim.throughput.csv $res_dir/redis/result-intel-sim.latency.csv \
    -is $res_dir/redis/result-intel-sgx.throughput.csv $res_dir/redis/result-intel-sgx.latency.csv
