#!/bin/sh

qemu-system-i386 -drive file=usb.image,index=0,media=disk,format=raw -netdev user,id=u1 -device e1000,netdev=u1 -object filter-dump,id=f1,netdev=u1,file=pcap.pcap
