#!/bin/sh

qemu-system-i386 -drive file=usb.image,index=0,media=disk,format=raw -netdev user,id=u1 -drive file=BuildImage,id=stick,format=raw,if=none -device e1000,netdev=u1 -object filter-dump,id=f1,netdev=u1,file=pcap.pcap -device usb-ehci,id=ehci -device usb-storage,bus=ehci.0,drive=stick -device usb-tablet,bus=ehci.0
