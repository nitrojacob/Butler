#!/bin/bash

git clone -b v2.6.0 https://github.com/ThrowTheSwitch/CMock cmock
git clone -b v2.6.0 https://github.com/ThrowTheSwitch/Unity unity

cmake .
make
./test_runner
