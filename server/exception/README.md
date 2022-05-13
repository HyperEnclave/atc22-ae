# Enclave Exception Test

## GU-Enclave

```
$ cd exception_gu_enclave
$ make SGX_MODE=HYPER
$ ./app 2000000
N = 2000000
Starting...
Register exception handler OK!
read OK: 0
write OK
Write Count = 32768
Total Time = 80050140 (cycles)
Average time = 2442.936 (cycles)

test #UD OK
Count = 2000000
Total Time = 33441907820 (cycles)
Average time = 16720.954 (cycles)

#UD Count = 2000000
Time elapsed: 8.359190 seconds
Done.
```

## P-Enclave

```
$ cd exception_p_enclave
$ make SGX_MODE=HYPER
$ ./app 100000000
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

## Intel SGX

```
$ cd exception_sgx_enclave
$ make SGX_MODE=HW
$ ./app 1000000
N = 1000000
test #UD OK
Count = 1000000
Total Time = 27842316540 (cycles)
Average time = 27842.317 (cycles)
Done.
```
