FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-c"]

RUN apt-get update; \
    apt-get install -y \
        autoconf \
        automake \
        build-essential \
        cmake \
        curl \
        git \
        jq \
        libssl-dev \
        libfuse-dev \
        libtool \
        ocaml \
        ocamlbuild \
        pkg-config \
        protobuf-compiler \
        python-is-python3 \
        sudo \
        unzip \
        vim \
        wget

# Install Rust
ENV PATH="/root/.cargo/bin:$PATH"
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | \
    sh -s -- --default-toolchain nightly-2021-11-01 -y && \
    cargo -V

WORKDIR /tmp

# Copy occlum and SGX SDK source
COPY ./archives.zip /tmp/
RUN unzip archives.zip && mv ./archives/* . && rm -rf archives archives.zip

# Install Occlum toolchain
ENV PATH="/opt/occlum/build/bin:/usr/local/occlum/bin:$PATH"
RUN cp -r ./occlum/tools/toolchains/musl-gcc /tmp/musl-gcc && \
    cd /tmp/musl-gcc && \
    ./build.sh

# Install Occlum Rust toolchain
ENV PATH="/opt/occlum/toolchains/rust/bin:$PATH"
RUN cp -r ./occlum/tools/toolchains/rust /tmp/rust && \
    cd /tmp/rust && \
    ./build.sh

# Build and install SGX SDK
RUN cd linux-sgx && \
    make preparation && \
    ./compile_and_install.sh && \
    echo 'source /opt/intel/sgxsdk/environment' >> /root/.bashrc

# Build and install Occlum
RUN cd occlum && \
    source /opt/intel/sgxsdk/environment && \
    OCCLUM_RELEASE_BUILD=1 SGX_MODE=HYPER make submodule && \
    OCCLUM_RELEASE_BUILD=1 OCCLUM_DISABLE_DCAP=1 SGX_MODE=HYPER make install

WORKDIR /home/admin/dev

# Copy SampleEnclave
RUN cp -r /opt/intel/sgxsdk/SampleCode/SampleEnclave /home/admin/dev/

# Copy benchmarks
COPY edge-calls /home/admin/dev/edge-calls
COPY nbench /home/admin/dev/nbench
COPY sqlite /home/admin/dev/sqlite
COPY lighttpd /home/admin/dev/lighttpd
COPY redis /home/admin/dev/redis
COPY exception /home/admin/dev/exception

# Setup benchmarks
RUN cd nbench && ./prepare.sh
RUN cd sqlite && ./prepare.sh
RUN cd lighttpd && ./prepare.sh
RUN cd redis && ./prepare.sh

# Build and install SGX SDK for EENTER/EEXIT benchmarks
COPY linux-sgx/eenter-eexit-stats.patch /tmp/linux-sgx/eenter-eexit-stats.patch
RUN cd /tmp/linux-sgx && \
    git apply eenter-eexit-stats.patch && \
    make USE_OPT_LIBS=3 sdk_no_mitigation && \
    make sdk_install_pkg_no_mitigation && \
    echo -e "no\n/opt/intel_sgxsdk_perf/" | sudo ./linux/installer/bin/sgx_linux_x64_sdk_*.bin

# Clean up
RUN cd /tmp && rm -rf musl-gcc rust linux-sgx occlum
