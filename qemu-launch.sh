#!/bin/sh

qemu-system-i386 -drive file=usb.image,index=0,media=disk,format=raw
