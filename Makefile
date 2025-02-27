#
# SCCS ID:	%W%	%G%
#
# Makefile to control the compiling, assembling and linking of standalone
# programs in the DSL.  Used for both individual interrupt handling
# assignments and the SP baseline OS (with appropriate tweaking).
#

#
# Application files
#

OS_C_SRC = clock.c kernel.c klibc.c kmem.c process.c \
	queues.c scheduler.c sio.c stacks.c syscalls.c pci.c \
	usb.c usb_uhci.c usbhd.c usbd.c

OS_C_OBJ = clock.o kernel.o klibc.o kmem.o process.o \
	queues.o scheduler.o sio.o stacks.o syscalls.o pci.o \
	usb.o usb_uhci.o usbhd.o usbd.o

OS_S_SRC = klibs.S
OS_S_OBJ = klibs.o

OS_LIBS =

OS_SRCS = $(OS_C_SRC) $(OS_S_SRC)
OS_OBJS = $(OS_C_OBJ) $(OS_S_OBJ)

USR_C_SRC = users.c ulibc.c
USR_C_OBJ = users.o ulibc.o

USR_S_SRC = ulibs.S
USR_S_OBJ = ulibs.o

USR_LIBS =

USR_SRCS = $(USR_C_SRC) $(USR_S_SRC)
USR_OBJS = $(USR_C_OBJ) $(USR_S_OBJ)

#
# Framework files
#

FMK_S_SRC = startup.S isr_stubs.S
FMK_S_OBJ = startup.o isr_stubs.o

FMK_C_SRC = cio.c support.c
FMK_C_OBJ = cio.o support.o

BOOT_SRC = bootstrap.S
BOOT_OBJ = bootstrap.o

FMK_SRCS = $(FMK_S_SRC) $(FMK_C_SRC)
FMK_OBJS = $(FMK_S_OBJ) $(FMK_C_OBJ)

# Collections of files

OBJECTS = $(FMK_OBJS) $(OS_OBJS) $(USR_OBJS)

SOURCES = $(BOOT_SRC) $(FMK_SRCS) $(OS_SRCS) $(USR_SRCS)

#
# Compilation/assembly definable options
#
# General options:
#	CLEAR_BSS		include code to clear all BSS space
#	GET_MMAP		get BIOS memory map via int 0x15 0xE820
#	SP_OS_CONFIG		enable SP OS-specific startup variations
#
# Debugging options:
#	CONSOLE_SHELL		compile in a simple shell for debugging
#	DEBUG_KMALLOC		debug the kernel allocator code
#	DEBUG_KMALLOC_FREELIST	debug the freelist creation
#	DEBUG_UNEXP_INTS	debug any 'unexpected' interrupts
#	REPORT_MYSTERY_INTS	print a message on interrupt 0x27 specifically
#	TRACE_CX		include context restore trace code
#	SANITY=n		enable "sanity check" level 'n'
#	  0			absolutely critical errors only
#	  1			important consistency checking
#	  2			less important consistency checking
#	  3			currently unused
#	  4			currently unused
#	STATUS=n		dump queue & process info every 'n' seconds
#

GEN_OPTIONS = -DCLEAR_BSS -DGET_MMAP -DSP_OS_CONFIG
DBG_OPTIONS = -DTRACE_CX -DCONSOLE_SHELL -DDEBUG_UNEXP_INTS

USER_OPTIONS = $(GEN_OPTIONS) $(DBG_OPTIONS)

#
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!!
#
# Compilation/assembly control
#

#
# We only want to include from the current directory and ~wrc/include
#
INCLUDES = -I. -I/home/fac/wrc/include

#
# Compilation/assembly/linking commands and options
#
CPP = cpp
CPPFLAGS = $(USER_OPTIONS) -nostdinc $(INCLUDES)

#
# Compiler/assembler/etc. settings for 32-bit binaries
#
CC = gcc
CFLAGS = -m32 -fno-pie -std=c99 -fno-stack-protector -fno-builtin -Wall -Wstrict-prototypes $(CPPFLAGS)

AS = as
ASFLAGS = --32

LD = ld
LDFLAGS = -melf_i386 -no-pie

#		
# Transformation rules - these ensure that all compilation
# flags that are necessary are specified
#
# Note use of 'cpp' to convert .S files to temporary .s files: this allows
# use of #include/#define/#ifdef statements. However, the line numbers of
# error messages reflect the .s file rather than the original .S file. 
# (If the .s file already exists before a .S file is assembled, then
# the temporary .s file is not deleted.  This is useful for figuring
# out the line numbers of error messages, but take care not to accidentally
# start fixing things by editing the .s file.)
#
# The .c.X rule produces a .X file which contains the original C source
# code from the file being compiled mixed in with the generated
# assembly language code.  Very helpful when you need to figure out
# exactly what C statement generated which assembly statements!
#

.SUFFIXES:	.S .b .X

.c.X:
	$(CC) $(CFLAGS) -g -c -Wa,-adhln $*.c > $*.X

.c.s:
	$(CC) $(CFLAGS) -S $*.c

.S.s:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S

.S.o:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(RM) -f $*.s

.s.b:
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(LD) $(LDFLAGS) -Ttext 0x0 -s --oformat binary -e begtext -o $*.b $*.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

#
# Targets for remaking bootable image of the program
#
# Default target:  usb.image
#

usb.image: bootstrap.b prog.b prog.nl BuildImage prog.dis 
	./BuildImage -d usb -o usb.image -b bootstrap.b prog.b 0x10000

floppy.image: bootstrap.b prog.b prog.nl BuildImage prog.dis 
	./BuildImage -d floppy -o floppy.image -b bootstrap.b prog.b 0x10000

prog.out: $(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.out $(OBJECTS)

prog.o:	$(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.o -Ttext 0x10000 $(OBJECTS) $(A_LIBS)

prog.b:	prog.o
	$(LD) $(LDFLAGS) -o prog.b -s --oformat binary -Ttext 0x10000 prog.o

#
# Targets for copying bootable image onto boot devices
#

floppy:	floppy.image
	dd if=floppy.image of=/dev/fd0

usb:	usb.image
	/usr/local/dcs/bin/dcopy usb.image

#
# Special rule for creating the modification and offset programs
#
# These are required because we don't want to use the same options
# as for the standalone binaries.
#

BuildImage:	BuildImage.c
	$(CC) -o BuildImage BuildImage.c

Offsets:	Offsets.c
	$(CC) -mx32 -std=c99 $(INCLUDES) -o Offsets Offsets.c

#
# Clean out this directory
#

clean:
	rm -f *.nl *.nll *.lst *.b *.o *.X *.image *.dis BuildImage Offsets

realclean:	clean

#
# Create a printable namelist from the prog.o file
#

prog.nl: prog.o
	nm -Bng prog.o | pr -w80 -3 > prog.nl

prog.nll: prog.o
	nm -Bn prog.o | pr -w80 -3 > prog.nll

#
# Generate a disassembly
#

prog.dis: prog.o
	objdump -d prog.o > prog.dis

#
# 'makedepend' is a program which creates dependency lists by looking
# at the #include lines in the source files.
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

bootstrap.o: bootstrap.h
startup.o: bootstrap.h
isr_stubs.o: bootstrap.h
cio.o: cio.h klib.h types.h support.h x86arch.h x86pic.h
support.o: support.h klib.h types.h cio.h x86arch.h x86pic.h bootstrap.h
support.o: process.h common.h udefs.h ulib.h stacks.h kmem.h queues.h
clock.o: x86arch.h x86pic.h ./x86pit.h common.h types.h udefs.h ulib.h klib.h
clock.o: clock.h process.h stacks.h kmem.h queues.h bootstrap.h scheduler.h
kernel.o: common.h types.h udefs.h ulib.h kernel.h x86arch.h process.h
kernel.o: stacks.h kmem.h queues.h bootstrap.h clock.h syscalls.h cio.h sio.h
kernel.o: scheduler.h users.h
klibc.o: common.h types.h udefs.h ulib.h
kmem.o: common.h types.h udefs.h ulib.h klib.h x86arch.h bootstrap.h kmem.h
kmem.o: cio.h
process.o: common.h types.h udefs.h ulib.h process.h stacks.h kmem.h queues.h
process.o: bootstrap.h
queues.o: common.h types.h udefs.h ulib.h queues.h process.h stacks.h kmem.h
queues.o: bootstrap.h
scheduler.o: common.h types.h udefs.h ulib.h scheduler.h
sio.o: common.h types.h udefs.h ulib.h ./uart.h x86arch.h x86pic.h sio.h
sio.o: queues.h process.h stacks.h kmem.h bootstrap.h scheduler.h kernel.h
sio.o: klib.h
stacks.o: common.h types.h udefs.h ulib.h stacks.h kmem.h
syscalls.o: common.h types.h udefs.h ulib.h x86arch.h x86pic.h ./uart.h
syscalls.o: support.h klib.h syscalls.h queues.h scheduler.h process.h
syscalls.o: stacks.h kmem.h bootstrap.h clock.h cio.h sio.h
users.o: common.h types.h udefs.h ulib.h users.h
ulibc.o: common.h types.h udefs.h ulib.h
ulibs.o: syscalls.h common.h types.h udefs.h ulib.h queues.h
