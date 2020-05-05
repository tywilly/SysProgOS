#!/bin/bash

# File: run.sh
# Author: Cody Burrows
# 
# A handy shell script for running our OS in QEMU with various options.

TTY=$(tty)
echo "Running from $TTY"

BASE="qemu-system-i386 -drive file=usb.image,index=0,media=disk,format=raw -serial mon:"$TTY" "
SOUND_PREFIX="-soundhw "
AC97="ac97"
SB="es1370"
CDROM="-cdrom /dev/cdrom "
USB="-usb -device usb-ehci,id=usb-ehci -device usb-storage,bus=usb-ehci.0,drive=usbstick "

# eval "$BASE""$SOUND_PREFIX""$AC97"",""$SB"" ""$CDROM""$USB"
eval "$BASE""$SOUND_PREFIX""$AC97"",""$SB"" ""$CDROM"

reset
