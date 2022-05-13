#!/bin/bash

http_server=${PWD}/lighttpd
set -e

unset LD_LIBRARY_PATH
source /opt/intel/sgxsdk/environment

# 1. Init Occlum Workspace
rm -rf occlum_workspace
mkdir occlum_workspace
cd occlum_workspace
occlum init

# 2. Copy files into Occlum Workspace and Build
cp $http_server image/bin
cp -r ../workspace/* image
occlum build
