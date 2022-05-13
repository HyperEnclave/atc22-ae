#!/bin/bash

RESULT=result-$1-$(date +%y%m%d-%H%M%S).txt

cd sgx-nbench && stdbuf -oL ./app | tee ../$RESULT
