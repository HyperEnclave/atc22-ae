#!/bin/bash
unset LD_LIBRARY_PATH
source /opt/intel_sgxsdk_perf/sgxsdk/environment
make clean
EENTER_EEXIT_STATS=1 SGX_MODE=HYPER make
./app hyper-eenter-eexit-$1
