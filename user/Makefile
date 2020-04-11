#
# SCCS ID:	%W%	%G%
#
# Makefile to control the compiling, assembling and linking of the userspace.
#

#
# Application files
#

OS_SRCS = $(OS_C_SRC) $(OS_S_SRC)
OS_OBJS = $(OS_C_OBJ) $(OS_S_OBJ)

ifeq ($(USER_TEST),y)
USR_C_SRC = users.c ulibc.c
USR_C_OBJ = users.o ulibc.o
else
USR_C_SRC = bin/userland.c lib/ulibc.c bin/consh.c
USR_C_OBJ = bin/userland.o lib/ulibc.o bin/consh.o
endif

USR_S_SRC = lib/ulibs.S
USR_S_OBJ = lib/ulibs.o

USR_LIBS =

USR_SRCS = $(USR_C_SRC) $(USR_S_SRC)
USR_OBJS = $(USR_C_OBJ) $(USR_S_OBJ)

# Collections of files

OBJECTS = $(USR_OBJS)

SOURCES = $(USR_SRCS)

USER_OPTIONS = $(GEN_OPTIONS) $(DBG_OPTIONS)

#
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!!
#
# Compilation/assembly control
#

#
# We only want to include from the current directory and ~wrc/include
#
INCLUDES = -I./include

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
	$(CC) $(CFLAGS) -c $*.c -o $*.o

#
# Targets for remaking bootable image of the program
#
# Default target:  usb.image
#

user.o:	$(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ -r $(OBJECTS)

#
# Clean out this directory
#

clean:
	rm -f *.nl *.nll *.lst *.b *.o *.X *.image *.dis BuildImage Offsets
	rm -f */*.o */*.lst

realclean:	clean

#
# 'makedepend' is a program which creates dependency lists by looking
# at the #include lines in the source files.
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

bin/userland.o: ./include/common.h ./include/types.h ./include/udefs.h
bin/userland.o: ./include/ulib.h bin/userland.h bin/consh.h
lib/ulibc.o: ./include/common.h ./include/types.h ./include/udefs.h
lib/ulibc.o: ./include/ulib.h
bin/consh.o: ./include/common.h ./include/types.h ./include/udefs.h
bin/consh.o: ./include/ulib.h bin/consh.h bin/userland.h
lib/ulibs.o: ./include/syscalls.h ./include/common.h ./include/types.h
lib/ulibs.o: ./include/udefs.h ./include/ulib.h
