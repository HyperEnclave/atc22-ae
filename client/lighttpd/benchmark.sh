#!/bin/bash

function usage() {
    echo "Usage: $0 [OPTIONS] <TAG>"
    echo ""
    echo "Options:"
    echo "    -h                      Display this message"
    echo "    -r <NUMBER>             Number of runs (default: 10)"
    echo "    -a <ADDRESS>:<PORT>     Address and port of the http server"
    echo "                            (default: 127.0.0.1:8000)"
    echo "    -n <REQUESTS>           Number of requests (default: 10000)"
    echo "    -c <CONCURRENCY>        Number of multiple requests to make at a time"
    echo "                            (default: 100)"
    echo ""
    echo "Arguments:"
    echo "    <TAG>                   Tag of this evalutaion"
    echo ""
}

while getopts "r:a:n:c:h" opt
do
   case "$opt" in
      r) runs="$OPTARG" ;;
      a) address="$OPTARG" ;;
      n) requests="$OPTARG" ;;
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
    runs=10
fi

if [ -z "$address" ]; then
    address="127.0.0.1:8000"
fi

if [ -z "$requests" ]; then
    requests=10000
fi

if [ -z "$concurrency" ]; then
    concurrency=100
fi

res_dir=result-$1-$(date +%y%m%d-%H%M%S)
sizes="1 10 20 30 40 50 60 70 80 90 100"
ab=ab
ab_args="-k -n $requests -c $concurrency"

declare -A all_tput
declare -A all_lat

mkdir -p "$res_dir"

for ((i=1; i<=$runs; i++))
do
    for n in $sizes
    do
        resfile=$res_dir/${n}_$i.res
        $ab $ab_args http://$address/${n}K.html > "$resfile" || exit $?

        tput=$(grep -m1 "Requests per second:" $resfile | awk '{ print $4 }')
        lat=$(grep -m1 "Time per request:" $resfile | awk '{ print $4 }')
        all_tput[$n]="${all_tput[$n]} $tput"
        all_lat[$n]="${all_lat[$n]} $lat"
        echo "[$i] bufsize=$n, throughput=$tput, latency=$lat"
        sleep 2
    done
done

function stats() {
    list=$(echo "$*" | tr " " ",")
    median=$(echo "$*" | tr " " "\n" | sort -n | awk '{a[NR]=$0} END {if(NR%2==1) print a[int(NR/2)+1]; else print(a[NR/2+1]+a[NR/2])/2}')
    average=$(echo "$*" | tr " " "\n" | awk '{sum+=$0; n++} END {print sum/n}')
}

echo "bufsize,$(seq -s, $runs),average,median" > $res_dir/throughput.csv
echo "bufsize,$(seq -s, $runs),average,median" > $res_dir/latency.csv
for n in $sizes
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
