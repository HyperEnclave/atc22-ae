#!/bin/bash

git clone https://github.com/utds3lab/sgx-nbench.git
cd sgx-nbench
git reset --hard 799f0fcd32d0f0392a3d3cd5b51455c48f121488
git apply ../sgx-nbench.patch
