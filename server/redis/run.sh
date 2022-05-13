#!/bin/bash

occlum_dir=/usr/local/occlum/x86_64-linux-musl
set -e

# 3. Run redis server
cd occlum_workspace
occlum run /bin/redis-server --save "" --appendonly no &
