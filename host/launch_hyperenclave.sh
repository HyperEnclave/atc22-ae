#!/bin/bash

if [ -z "$1" -o -z "$2" ]; then
    echo "Usage: $0 <rust_monitor.elf> <hyper_enclave.ko>"
    exit 1
fi

hv_img=$1
mod_file=$2
CMDLINE=/proc/cmdline

function get_memmap_parameter()
{
	for i in $(cat $CMDLINE)
	do
		if [[ "$i" =~ "memmap=" ]];then
			# multi memory ranges
			str_mmap=($(echo $i | awk -F"[=,]" '{for(i=2;i<=NF;i++)printf("%s ",$i);}'))
			break
		fi
	done

	flag=1
	for((i=0;i<${#str_mmap[*]};i++));
	do
		str=${str_mmap[$i]}
		tmp=$(echo $str | awk -F"[$ ]" '/[0-9a-fA-FxX]+[KMG]?\$[0-9a-fA-FxX]+[KMG]?/\
			{printf("%s,%s",$2,$1);}')

		if [ "$tmp" != "" ];then
			if [ $flag -eq 1 ];then
				res+=$tmp
				flag=0
			else
				res+=,$tmp
			fi
		fi
	done

	if [ "$res" = "" ];then
		echo "ERROR"
	else
		echo "$res"
	fi
}

sudo cp $hv_img /lib/firmware/rust-monitor-amd
if [ ! -z "$(lsmod | grep hyper_enclave)" ]; then
	sudo sysctl dev.hyper_enclave.enabled=0
	sudo rmmod hyper_enclave
fi

res=$(get_memmap_parameter)
if [ "$res" = "ERROR" ];then
	echo "ERROR. Please use correct memmap parameter: memmap=nn[KMG]$ss[KMG]"
else
	# memmap=63G$0x1080000000,63G$0x2080000000
	# sudo insmod $mod_file str_memmap=0x1080000000,63G,0x2080000000,63G
	sudo insmod $mod_file str_memmap=$res
	sudo sysctl dev.hyper_enclave.enabled=1
fi
