#!/bin/bash

function usage() {
    echo "Usage: $0 [OPTIONS] <TAG>"
    echo ""
    echo "Options:"
    echo "    -h                      Display this message"
    echo "    -r <NUMBER>             Number of runs (default: 6)"
    echo "    -a <ADDRESS>            Redis host (default: 127.0.0.1)"
    echo "    -p <PORT>               Redis port (default: 6379)"
    echo "    -w <WORKLOAD>           Workload ID in A, B, C, D, E, F (default: A)"
    echo "    -n <REC_COUNT>          Number of records (default: 50000)"
    echo "    -t <OP_COUNT>           Number of operations (default: 100000)"
    echo "    -c <CONCURRENCY>        Number of multiple requests to make at a time"
    echo "                            (default: 20)"
    echo ""
    echo "Arguments:"
    echo "    <TAG>                   Tag of this evalutaion"
    echo ""
}

while getopts "r:a:p:w:n:t:c:h" opt
do
   case "$opt" in
      r) runs="$OPTARG" ;;
      a) address="$OPTARG" ;;
      p) port="$OPTARG" ;;
      n) rec_count="$OPTARG" ;;
      t) op_count="$OPTARG" ;;
      c) concurrency="$OPTARG" ;;
      h | ?) usage; exit 0 ;;
   esac
done

shift $((OPTIND - 1))
if [ -z "$1" ]; then
    usage
    exit 0
fi

if [ -z "$runs" ]; then
    runs=6
fi

if [ -z "$address" ]; then
    address="127.0.0.1"
fi

if [ -z "$port" ]; then
    port="6379"
fi

if [ -z "$rec_count" ]; then
    rec_count=50000
fi

if [ -z "$op_count" ]; then
    op_count=100000
fi

if [ -z "$concurrency" ]; then
    concurrency=20
fi

if [ -z "$workload" ]; then
    workload=A
fi

case $workload in
    a | A) workload=workloada ;;
    b | B) workload=workloadb ;;
    c | C) workload=workloadc ;;
    d | D) workload=workloadd ;;
    e | E) workload=workloade ;;
    f | F) workload=workloadf ;;
    *) usage; exit 0 ;;
esac

if [ -z "$REDIS_CLI" ]; then
    REDIS_CLI=redis-cli
fi

if [ -z "$YCSB_ROOT" ]; then
    echo "Undefined environment variable: YCSB_ROOT"
    exit 0
fi

res_dir=result-$1-$(date +%y%m%d-%H%M%S)
targets="5000 10000 15000 20000 25000 30000 35000 40000"

ycsb=$YCSB_ROOT/bin/ycsb.sh
workload=$YCSB_ROOT/workloads/$workload
load_args="-p redis.host=$address -p redis.port=$port -p recordcount=$rec_count -threads 10 -s"
run_args="-p redis.host=$address -p redis.port=$port -p operationcount=$op_count -threads $concurrency -s"

declare -A all_tput
declare -A all_lat

mkdir -p "$res_dir"

for ((i=1; i<=$runs; i++))
do
    for n in $targets
    do
        echo "Target throughput: $n"
        resfile=$res_dir/${n}_$i.res

        $REDIS_CLI flushall
        $ycsb load redis -P $workload $load_args > "$res_dir/.load.res" || exit $?
        $REDIS_CLI dbsize
        sleep 2
        $ycsb run redis -P $workload $run_args -target $n > "$resfile" || exit $?

        read_count=$(grep -m1 "\[READ\], Operations" $resfile | awk '{ print $3 }')
        read_lat=$(grep -m1 "\[READ\], AverageLatency" $resfile | awk '{ print $3 }')
        update_count=$(grep -m1 "\[UPDATE\], Operations" $resfile | awk '{ print $3 }')
        update_lat=$(grep -m1 "\[UPDATE\], AverageLatency" $resfile | awk '{ print $3 }')
        failed_count=$(grep -m1 "\[READ-FAILED\], Operations" $resfile | awk '{ print $3 }')
        failed_lat=$(grep -m1 "\[READ-FAILED\], AverageLatency" $resfile | awk '{ print $3 }')

        if [ -z "$failed_count" ]; then
            failed_count=0
            failed_lat=0
        fi

        echo "round=$i, target=$n: [READ] count=$read_count, latency=$read_lat"
        echo "round=$i, target=$n: [UPDATE] count=$update_count, latency=$update_lat"
        echo "round=$i, target=$n: [READ-FAILED] count=$failed_count, latency=$failed_lat"

        tput=$(grep -m1 "Throughput" $resfile | awk '{ print $3 }')
        lat=$(awk "BEGIN{print ($read_count * $read_lat + $update_count * $update_lat + $failed_count * $failed_lat) / ($read_count + $update_count + $failed_count)}")
        all_tput[$n]="${all_tput[$n]} $tput"
        all_lat[$n]="${all_lat[$n]} $lat"
        echo "[$i] target=$n, Throughput(ops/sec)=$tput, AverageLatency(us)=$lat"
        sleep 2
    done
done

function stats() {
    list=$(echo "$*" | tr " " ",")
    median=$(echo "$*" | tr " " "\n" | sort -n | awk '{a[NR]=$0} END {if(NR%2==1) print a[int(NR/2)+1]; else print(a[NR/2+1]+a[NR/2])/2}')
    average=$(echo "$*" | tr " " "\n" | awk '{sum+=$0; n++} END {print sum/n}')
}

echo "target_throughput,$(seq -s, $runs),average,median" > $res_dir/throughput.csv
echo "target_throughput,$(seq -s, $runs),average,median" > $res_dir/latency.csv
for n in $targets
do
    stats ${all_tput[$n]}
    echo "$n,$list,$average,$median" >> $res_dir/throughput.csv
    stats ${all_lat[$n]}
    echo "$n,$list,$average,$median" >> $res_dir/latency.csv
done

echo ""
echo "Summary (Throughput):"
cat $res_dir/throughput.csv
echo ""
echo "Summary (Latency):"
cat $res_dir/latency.csv
