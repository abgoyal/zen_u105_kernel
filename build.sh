#! /bin/bash

ARCH=arm CROSS_COMPILE=../arm-eabi-4.4.3/bin/arm-eabi- KERNEL_SOURCE=. make sp6821a-vlx_defconfig Image modules


