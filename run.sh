#!/bin/bash

# File: run.sh
# Author: Cody Burrows
# 
# A handy shell script for running our OS in QEMU with various options.

TTY=$(tty)
echo "Running from $TTY"

BASE="qemu-system-i386 -drive file=usb.image,index=0,media=disk,format=raw "
BASE+="-serial mon:"$TTY" "
SOUND_PREFIX="-soundhw "
AC97="ac97"
SB="es1370"
CDROM="-cdrom /dev/cdrom "
USB="-drive if=none,id=usbstick,file=flashdrive.img,format=raw "
USB+="-usb -device usb-ehci,id=usb-ehci "
USB+="-device usb-storage,bus=usb-ehci.0,drive=usbstick "
USAGE="Usage: ./run.sh [usb, cdrom]"
COMMAND="$BASE""$SOUND_PREFIX""$AC97"",""$SB"" "

# a fine variable for perusing the command line arguments
for foo in "$@"
do
    if [ "$foo" = "usb" ]; then
        COMMAND+="$USB"
    elif [ "$foo" = "cdrom" ]; then
        COMMAND+="$CDROM"
    else
        echo "$USAGE" && exit
    fi
done

eval "$COMMAND"

# QEMU leaves my terminal windows unusable for some reason. If there wasn't
# an error status, reset it so it's usable again.
if [ "$?" = 0 ]; then
    reset
fi
