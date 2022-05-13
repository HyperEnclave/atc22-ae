#!/bin/bash

occlum_dir=/usr/local/occlum/x86_64-linux-musl
set -e

unset LD_LIBRARY_PATH
source /opt/intel/sgxsdk/environment

# 1. Init Occlum Workspace
rm -rf occlum_workspace
occlum new occlum_workspace
cd occlum_workspace
new_json="$(jq '.resource_limits.user_space_size = "320MB" |
                .process.default_mmap_size = "256MB"' Occlum.json)" && \
echo "${new_json}" > Occlum.json

# 2. Copy files into Occlum Workspace and Build
cp $occlum_dir/redis/bin/* image/bin
cp $occlum_dir/lib/libssl* image/lib
cp $occlum_dir/lib/libcrypto* image/lib
occlum build
