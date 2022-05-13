#!/bin/bash

https_server=lighttpd
set -e

# 3. Run https_server
cd occlum_workspace
occlum run /bin/$https_server -D -f /lighttpd.conf
