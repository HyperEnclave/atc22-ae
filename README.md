# HyperEnclave ATC '22 Artifact Evaluation

Yuekai Jia, Shuang Liu, Wenhao Wang, Yu Chen, Zhengde Zhai, Shoumeng Yan, and Zhengyu He. [HyperEnclave: An Open and Cross-platform Trusted Execution Environment](https://www.usenix.org/conference/atc22/presentation/jia-yuekai). In *2022 USENIX Annual Technical Conference (USENIX ATC 22)*, pages 437-454, Carlsbad, CA, July 2022. USENIX Association.

```
@inproceedings {jia2022hyperenclave,
  author = {Yuekai Jia and Shuang Liu and Wenhao Wang and Yu Chen and Zhengde Zhai and Shoumeng Yan and Zhengyu He},
  title = {{HyperEnclave}: An Open and Cross-platform Trusted Execution Environment},
  booktitle = {2022 USENIX Annual Technical Conference (USENIX ATC 22)},
  year = {2022},
  isbn = {978-1-939133-29-48},
  address = {Carlsbad, CA},
  pages = {437--454},
  url = {https://www.usenix.org/conference/atc22/presentation/jia-yuekai},
  publisher = {USENIX Association},
  month = jul,
}
```

## 1. Overview

### 1.1 Artifact directory layout

* `README.md`: describes the artifact and provides a road map for evaluation.
* `host/`: contains RustMonitor binary, the Linux kernel module binary, and the scripts to install and enable HyperEnclave.
* `server/`: contains the source code (or patches) and scripts of all experiments to run within the enclaves. We also provide a docker container with all dependencies installed.
* `client/`: contains the benchmark scripts for network-based experiments (Lighttpd and Redis) to run on the client-side. We also provide a docker container with all dependencies installed.
* `plots/`: contains plotting scripts to generate figures from the experiment results.
* `paper-results/`: contains the results shown in the paper.

### 1.2 Main experiments

| Experiments | Figure/Table | Which container | Run time | Description |
|------------|--------------|-----------------|----------|-------------|
| edge-calls | Table 1      | server          | 10s      | `EENTER`/`EEXIT` and ECALLs/OCALLs latency. |
| exception  | Table 2      | server          | 20s      | Exception handling inside the enclaves. |
| NBench     | Figure 8a    | server          | 10m      | Performance scores of [NBench](https://www.math.utah.edu/~mayer/linux/bmark.html) inside enclaves. |
| SQLite     | Figure 8b    | server          | 15m      | Throughput of in-memory [SQLite](https://www.sqlite.org) database with different number of records, under [YCSB](https://github.com/brianfrankcooper/YCSB/wiki) A workload.
| Lighttpd   | Figure 8c    | server/client   | 10m      | Throughput of [Lighttpd](https://www.lighttpd.net) web server inside [Occlum](https://github.com/occlum/occlum) LibOS with different request sizes. The client uses [ab](https://httpd.apache.org/docs/2.4/programs/ab.html).
| Redis      | Figure 8d    | server/client   | 20m      | Latency-throughput curve of [Redis](https://redis.io) in-memory database server inside Occlum LibOS with increasing request frequencies. The client uses YCSB A workload.

## 2. System Requirements

* Hardware:
    1. A 64-bit AMD platform with SVM (Secure Virtual Machine). Optionally, we recommend that the platform should support SME (Secure Memory Encryption) for the protection against physical memory attacks.
    2. RAM >= 16 GB
    3. Free disk space >= 30 GB

    > We disabled TPM and IOMMU features in RustMonitor binary for artifact evaluation to minimize the hardware requirements. These features do not affect the performance results.

* Software:
    1. Linux with the specified kernel version (i.e., `5.3.0-28-generic`) to match our given kernel module binary. We recommend [Ubuntu 18.04.4 LTS](https://old-releases.ubuntu.com/releases/18.04.4/ubuntu-18.04.4-desktop-amd64.iso) which uses this version of kernel as the default.
    2. [Docker](https://docs.docker.com/engine/install/ubuntu/) (any recent version).
    3. [git](https://git-scm.com/download/linux) (for cloning the AE repository).
    4. GCC and Linux kernel headers (for [building](https://askubuntu.com/questions/17944/what-is-the-minimum-requirement-to-compile-kernel-modules) the [enable_rdfsbase](https://github.com/occlum/enable_rdfsbase) kernel module).

## 3. Getting Started

### 3.0 Check requirements

1. Hardware:

    Make sure your CPU supports SVM and SME features. If does, the output of the following commands is not empty:

    ```console
    $ cat /proc/cpuinfo | grep -w svm
    $ cat /proc/cpuinfo | grep -w sme
    ```

 	Check whether BIOS enabled SME by reading SYSCFG MSR bit 23 (MemEncryptionModEn), `1` means SME is enabled:

    ```console
    $ sudo apt-get install msr-tools
    $ sudo modprobe msr
    $ sudo rdmsr 0xC0010010 -X -f 23:23
    1
    ```

    If SME is not available, you can continue running experiments with our no-SME version binaries (Section 3.1).

2. Software:

    Make sure your kernel version is `5.3.0-28-generic`, or you can manually install it via `apt-get`:

    ```console
    $ sudo apt-get install linux-image-5.3.0-28-generic
    $ uname -r
    5.3.0-28-generic
    ```

    Make sure you can use the following commands: `git`, `make`, `gcc`, `docker`, and the directory `/lib/modules/5.3.0-28-generic` exists.

### 3.1 Clone the AE repository (less than 1 minute)

```console
$ git clone https://github.com/HyperEnclave/atc22-ae.git
```

### 3.2 Configure kernel command-line parameters (about 3 minutes)

1. Open `/etc/default/grub`, add or modify the line:

    ```
    GRUB_CMDLINE_LINUX_DEFAULT="console=tty0 console=ttyS0,115200n8 memmap=6G\\\$0x100000000 amd_iommu=off mem_encrypt=off"
    ```

    This configures the serial port output, reserves RustMonitor and EPC memory (6 GB), and disables IOMMU and memory encryption in Linux (allows RustMonitor to control them).

2. Generate the new grub config file and reboot your system:

    ```console
    $ sudo update-grub2
    $ sudo reboot
    ```

3. Verify the grub change was applied after the system reboot:

 	```console
 	$ cat /proc/cmdline
 	BOOT_IMAGE=/boot/vmlinuz-5.3.0-28-generic root=UUID=75d32e81-f1fc-4b6b-8ee7-dbc7595fa081 ro console=tty0 console=ttyS0,115200n8 memmap=6G$0x100000000 amd_iommu=off mem_encrypt=off quiet splash
 	```

### 3.3 Install and launch HyperEnclave (about 3 minutes)

1. Install the [enable_rdfsbase](https://github.com/occlum/enable_rdfsbase) kernel module to enable the `RDFSBASE` family instructions to run [Occlum](https://github.com/occlum/occlum):

    ```console
    $ git clone https://github.com/occlum/enable_rdfsbase.git
    $ cd enable_rdfsbase && make && make install
    ```

    If `dmesg` outputs the following information without errors, the installation is successful:

    ```console
    $ dmesg
    [381479.980251] enable_rdfsbase: Loaded
    [381479.980265] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 0
    [381479.980270] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 1
    [381479.980275] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 2
    [381479.980279] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 3
    [381479.980289] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 4
    [381479.980295] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 5
    [381479.980297] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 6
    [381479.980298] enable_rdfsbase: RDFSBASE and its friends are now enabled on CPU 7
    ```

2. Launch RustMonitor to deprivilege Linux to the guest mode:

    ```console
    $ cd host
    $ ./launch_hyperenclave.sh binaries/rust_monitor_sme.elf binaries/hyper_enclave_5.3.0-28-generic.ko
    dev.hyper_enclave.enabled = 1
    ```

    Make sure the suffix of the kernel module matches your kernel version, and SME is available and enabled by your BIOS. If your platform does not support SME, you can use the no-SME version of RustMonitor binary `rust_monitor_no_sme.elf`.

    If the above command returns and `dmesg` outputs the following information without errors, RustMonitor launched successfully:

    ```console
    $ dmesg
    [534307.259883] memmap range: [0x100000000-0x27fffffff], 0x180000000
    [534307.266689] hypervisor size: 0x0
    [534307.270477] SME mask: [0x800000000000]
    [534307.283461] hyperenclave_cmd_enable
    [534307.288956] SME mask: [0x800000000000]
    [534307.293249] hypervisor size: 0x80000000
    [534307.606589] config_size: 1290
    [534307.649148] add_epc_pages. total_epc_pages: 0x100000, free_epc_pages: 0x100000
    [534307.657317] epc ranges: [0x180000000-0x27fffffff], 0x100000000
    [534307.663932] config_header load_addr: 0xffffff0030e0c000
    [534307.669867] mem_region load_addr: 0xffffff0030e0c18a
    [534307.741779] C2843949C5654F443E10A21A5E1AF68F03A435DDF0FF6DA0BC588AB8C1CCF523
    [534309.121640] tpm_pcr_extend result=0
    [534329.992768] HyperEnclave is opening.
    ```

3. You can use the tool `test_hypercall` to make sure you are now in the guest mode:

    ```console
    $ cd host
    $ ./test_hypercall
    Execute VMMCALL OK.
    You are in the Guest mode.
    ```
4. (optional) You can further stop RustMonitor and promote Linux to the host mode:

    ```console
    $ cd host
    $ ./stop_hyperenclave.sh
    dev.hyper_enclave.enabled = 0
    $ dmesg
    [ 383788.404403] HyperEnclave was closed.
    ```

### 3.4 Setup containers (about 5 minutes)

We provided two containers to reduce the environment configuration efforts for experiments. The `server` container pre-installed our enclave SDK, adapted Occlum LibOS, all experiments, and their dependencies. The `client` container pre-installed client-side benchmark scripts for Lighttpd and Redis.

1. Launch the `server` container:

    ```console
    $ cd server
    $ make start
    $ make enter
    root@your-machine-name:/home/admin/dev#
    ```

2. Launch the `client` container:

    ```console
    $ cd client
    $ make start
    $ make enter
    root@your-machine-name:~#
    ```

### 3.5 Run Hello-world Example (less than 1 minute)

SampleEnclave is a "Hello world"-sized example for the kick-the-tires. You can quickly compile and run it in the `server` container:

```console
$ cd SampleEnclave
$ SGX_MODE=SIM make # build for simulation mode to run without security guarantees (baseline)
$ ./app
...
Info: executing thread synchronization, please wait...
Info: SampleEnclave successfully returned.
$ SGX_MODE=HYPER make   # build for hyper mode to run on HyperEnclave with security guarantees
$ ./app
...
Info: executing thread synchronization, please wait...
Info: SampleEnclave successfully returned.
```

The message "Info: SampleEnclave successfully returned" indicates it ran successfully.

## 4. Run Experiments

Our enclave SDK was based on [Intel SGX SDK](https://github.com/intel/linux-sgx). SGX applications can be built in **Simulation mode**, which can run on the non-SGX platform but with no security guarantees. In our artifact, we add a **Hyper mode** in the SDK to run SGX applications securely in HyperEnclave and **use the simulation mode as the baseline**. An application can easily switch between the two modes by recompiling with the environment variable `SGX_MODE`: `SGX_MODE=SIM` for simulation mode, and `SGX_MODE=HYPER` for hyper mode.

At least, you need to run all experiments twice, one in simulation mode, and one in hyper mode, to generate the **baseline** results and the **GU-Enclave** variant results. Furthermore, we recommend running the edge-calls, Lighttpd, and Redis experiments using the **HU-enclave** variant, the performance will be significantly improved. The exception experiment is only runnable for GU-Enclave and **P-Enclave**, We also recommend running it to demonstrate the advantages of P-Enclaves.

<!-- Note that the baseline results should be evaluated without HyperEnclave enabled to avoid the virtualization overhead. (you can stop HyperEnclave by the script `host/stop_hyperenclave.sh`) -->

### 4.1 edge-calls

In the `server` container:

```console
$ cd edge-calls
$ SGX_MODE=<MODE> ./build.sh    # <MODE>=SIM and HYPER respectively
$ ./app <tag>                   # <tag>=sim and hyper respectively
Hello world!
Benchmark rdtscp in App:
Median = 60.000
Average = 52.631
Min = 40.000
Max = 80.000
Deviation = 18.869

Benchmark ecall_empty in App:
Median = 9580.000
Average = 9828.510
Min = 9280.000
Max = 1180560.000
Deviation = 2129.963

Benchmark ocall_empty in Enclave:
Median = 4920.000
Average = 5381.095
Min = 4800.000
Max = 1152800.000
Deviation = 4619.698

rdtcsp: 60
ecall: 9580
ocall: 4920
```

Where `<tag>` indicates the result tag (e.g., use `sim` for baseline results, use `hyper` for HyperEnclave results, `p` for P-Enclaves, and `h` for HU-Enclaves). The results will be output to the console and saved in the directory with format `result-<tag>-<timestamp>`.

To benchmark the latency of `EENTER` and `EEXIT` instructions, we added some statistics instructions to the SDK during world switches (see `eenter_eexit_stats.patch`), which may introduce small overheads. You need to switch to the SDK installed in `/opt/intel_sgxsdk_perf` to build the experiment code, or just run the script:

```console
$ ./bench_hyper_eenter_eexit.sh <tag>
......
  eenter_count = 2020007
  eenter_cycles = 3424838840
EENTER latency = 1695.459
  eexit_count = 2020007
  eexit_cycles = 2695887600
EEXIT latency = 1334.593
```

### 4.2 NBench

In the `server` container:

```console
$ cd nbench
$ SGX_MODE=<MODE> ./build.sh
$ ./benchmark.sh <tag>
====================Modified version of nbench for Intel SGX====================
TEST                : Iterations/sec.  : Old Index   : New Index
                    :                  : Pentium 90* : AMD K6/233*
--------------------:------------------:-------------:------------
NUMERIC SORT        :          1134.7  :      29.10  :       9.56
STRING SORT         :          269.47  :     120.41  :      18.64
BITFIELD            :      3.0734e+08  :      52.72  :      11.01
FP EMULATION        :          369.78  :     177.44  :      40.94
FOURIER             :           29079  :      33.07  :      18.58
ASSIGNMENT          :          41.774  :     158.96  :      41.23
IDEA                :           10018  :     153.23  :      45.49
HUFFMAN             :          3458.3  :      95.90  :      30.62
NEURAL NET          :          60.217  :      96.73  :      40.69
LU DECOMPOSITION    :            1628  :      84.34  :      60.90
=================================TEST COMPLETED=================================
```

The results will be output to the console and saved to the file `result-<tag>-<datetime>.txt`.

### 4.3 SQLite

In the `server` container:

```console
$ cd sqlite
$ SGX_MODE=<MODE> ./build.sh
$ ./benchmark.sh <tag>
[1] Record count: 5000, Throughput (KTPS): 156.7634
[1] Record count: 10000, Throughput (KTPS): 154.3724
[1] Record count: 20000, Throughput (KTPS): 146.6254
[1] Record count: 40000, Throughput (KTPS): 143.6196
......
```

It will run several rounds and output throughput for all rounds and record counts in the CSV format. The CSV will also be saved to the file `result-<tag>-<datetime>/throughput.csv`. More detailed results can also be found in this directory.

You can further change the benchmark arguments, run `./benchmark.sh --help` for more details.

### 4.4 Lighttpd

In the `server` container:

```console
$ cd lighttpd
$ SGX_MODE=<MODE> ./build.sh
$ ./run.sh
2022-05-11 16:40:31: (log.c.196) server started
```

Now the console in the `server` container will be blocked to wait for the requests from the client, and you may enter the `client` container to run the benchmark:

```console
$ cd lighttpd
$ ./benchmark.sh <tag>
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests
[1] bufsize=1, throughput=13096.39, latency=7.636
......
```

It will run several rounds and output the throughput CSV as well as the latency CSV, you can also found them in `result-<tag>-<datetime>/throughput.csv` and `result-<tag>-<datetime>/latency.csv`.

Run `./benchmark.sh --help` for more arguments usage.

### 4.5 Redis

In the `server` container:

```console
$ cd redis
$ SGX_MODE=<MODE> ./build.sh
$ ./run.sh
2:C 11 May 2022 16:50:45.545 # oO0OoO0OoO0Oo Redis is starting oO0OoO0OoO0Oo
2:C 11 May 2022 16:50:45.545 # Redis version=6.0.9, bits=64, commit=25214bd7, modified=0, pid=2, just started
2:C 11 May 2022 16:50:45.545 # Configuration loaded
2:M 11 May 2022 16:50:45.547 # You requested maxclients of 10000 requiring at least 10032 max file descriptors.
2:M 11 May 2022 16:50:45.547 # Server can't set maximum open files to 10032 because of OS error: Operation not permitted.
2:M 11 May 2022 16:50:45.547 # Current maximum open files is 1024. maxclients has been reduced to 992 to compensate for low ulimit. If you need higher maxclients increase 'ulimit -n'.
                _._
           _.-``__ ''-._
      _.-``    `.  `_.  ''-._           Redis 6.0.9 (25214bd7/0) 64 bit
  .-`` .-```.  ```\/    _.,_ ''-._
 (    '      ,       .-`  | `,    )     Running in standalone mode
 |`-._`-...-` __...-.``-._|'` _.-'|     Port: 6379
 |    `-._   `._    /     _.-'    |     PID: 2
  `-._    `-._  `-./  _.-'    _.-'
 |`-._`-._    `-.__.-'    _.-'_.-'|
 |    `-._`-._        _.-'_.-'    |           http://redis.io
  `-._    `-._`-.__.-'_.-'    _.-'
 |`-._`-._    `-.__.-'    _.-'_.-'|
 |    `-._`-._        _.-'_.-'    |
  `-._    `-._`-.__.-'_.-'    _.-'
      `-._    `-.__.-'    _.-'
          `-._        _.-'
              `-.__.-'

2:M 11 May 2022 16:50:45.547 # Server initialized
2:M 11 May 2022 16:50:45.548 * Ready to accept connections
```

To benchmark in the `client` container, you need to specify the YCSB installation path:

```console
$ cd redis
$ YCSB_ROOT=../tools/ycsb-redis-binding-0.17.0 ./benchmark.sh <tag>
Target throughput: 5000
OK
Command line: -load -db site.ycsb.db.RedisClient -P ../tools/ycsb-redis-binding-0.17.0/workloads/workloada -p redis.host=127.0.0.1 -p redis.port=6379 -p recordcount=50000 -threads 10 -s
YCSB Client 0.17.0

Loading workload...
Starting test.
2022-05-11 17:19:19:658 0 sec: 0 operations; est completion in 0 second
......
```

It will run several rounds and output the throughput CSV as well as the latency CSV, you can also found them in `result-<tag>-<datetime>/throughput.csv` and `result-<tag>-<datetime>/latency.csv`.

Run `./benchmark.sh --help` for more arguments usage.

Finally, you need to run `redis-cli shutdown` in the `client` container to stop the Redis server.

### 4.6 Exception handling benchmark

This experiment compares the exception handling performance between GU-Enclave and P-Enclave. You can see there are two sub-directory `exception_gu_enclave` and `exception_p_enclave`, make sure you are using the correct directory according to the current enclave operation mode. Please see Section 5 for how to switch enclave operation modes.

In the `server` container:

```console
$ cd exception/exception_p_enclave      # cd exception/exception_gu_enclave for GU-Enclave
$ make SGX_MODE=HYPER
$ ./app 100000000                       # Change to smaller (e.g., 2000000) for GU-Enclave
N = 100000000
Starting...
Register exception handler OK!
CR3 = 0x13163a000
*CR3 = 0x0
read OK: 0
write OK
Write Count = 32768
Total Time = 41520900 (cycles)
Average time = 1267.117 (cycles)

test #UD OK
Count = 100000000
Total Time = 32265510580 (cycles)
Average time = 322.655 (cycles)

Interrupt Count = 0
#UD Count = 100000000
#PF Count = 32768

Time elapsed: 8.044846 seconds
Done.
```

The result shows the average handling time for `#PF` and `#UD` exceptions. P-Enclave results are expected to be 2x and 60x faster than GU-Enclave results for the two exceptions, respectively.

### 4.7 Plot the result

1. Install `python3` and dependencies:

    ```console
    $ cd plots
    $ pip3 install -r requirements.txt
    ```

2. Plot figures from the results in the paper:

    ```console
    $ ./gen_paper_results.sh
    ```

3. Plot figures from your results:

    1. All results are generated in the container, you should first take them out of the container. You can run the following command in the host to copy files from the container:

        ```console
        $ docker cp container_id:/path/to/your/result .
        ```

    2. Modify the script `gen_paper_results.sh`, replace the result file paths by your own. You don't need to fill in all arguments, if some arguments are empty, the corresponding curves will not be plotted.

## 5. Flexible Enclave Operation Mode

Currently, we support flexible enclave modes by providing separate RustMonitor binaries, which are located at the directory `host/binaries/`:

| Binaries | Which operation mode | SME |
|----------|----------------------|-----|
| `rust_monitor_no_sme.elf`     | GU-Enclave | off |
| `rust_monitor_no_sme_hu.elf`  | HU-Enclave | off |
| `rust_monitor_no_sme_p.elf`   | P-Enclave  | off |
| `rust_monitor_sme.elf`        | GU-Enclave | on  |
| `rust_monitor_sme_hu.elf`     | HU-Enclave | on  |
| `rust_monitor_sme_p.elf`      | P-Enclave  | on  |

You can switch to a specific mode by re-launching the corresponding RustMonitor:

```console
$ ./launch_hyperenclave.sh binaries/rust_monitor_xxx.elf binaries/hyper_enclave_5.3.0-28-generic.ko
```

### 5.1 P-Enclave

All experiments in Section 4 can run in P-Enclaves without rebuilding. You will see that P-Enclaves perform similarly to GU-Enclaves (except in the exception handling benchmark).

### 5.2 HU-Enclave

Since we replaced hypercall instructions with system call instructions in HU-Enclaves, it uses a different enclave SDK from GU-Enclaves and P-Enclaves. Furthermore, the SDK libraries used for Occlum are different, too. So we use another `server` container that pre-installed all dependencies for HU-Enclaves. (No need for another `client` container)

You can easily use the `server` container for HU-Enclaves by setting an argument in the script:

```console
$ cd server
$ make start HU=1
$ make enter HU=1
root@your-machine-name:/home/admin/dev#
```

Then run all experiments in Section 4 to get the performance results for HU-Enclaves. You will see that HU-Enclaves perform better than GU-Enclaves and P-Enclaves in the same experiment.
