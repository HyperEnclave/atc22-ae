# SGX Edge Call Latency Benchmarks

## Build

Build with Intel SGX SDK:

```bash
SGX_MODE=<MODE> make    # MODE = SIM, HYPER, HW
```

To build for Intel SGX platform, set the environment variable `ENCLAVE_RDTSC=0`.

Build syscall/hypercall benchmarks without Intel SGX SDK:

```bash
SGX_MODE= make native
```

## Run

```bash
./app
# or ./native
```
