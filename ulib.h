/*
** SCCS ID:	@(#)ulib.h	1.1	3/30/20
**
** File:	ulib.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	Declarations for user-level library functions
*/

#ifndef _ULIB_H_
#define _ULIB_H_

#include "types.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/
#define NULL 0
#define stdin CHAN_SIO
#define stdout CHAN_SIO
#define stderr CHAN_CONS

/*
** Prototypes
*/

/*
**********************************************
** SYSTEM CALLS
**********************************************
*/

/*
** exit - terminate the calling process
**
** usage:	exit(status);
**
** @param status Terminations tatus of this process
**
** @returns Does not return
*/
void exit( int32 status );

/*
** kill - terminate a process with extreme prejudice
**
** usage:	n = kill(pid);
**
** @param pid The PID of the desired victim, or 0 for this process
**
** @returns 0 on success, else < 0 on an error, unless pid was 0
*/
int32 kill( Pid pid );

/*
** wait - wait for a child process to terminate
**
** usage:	pid = wait(pid,&status);
**
** @param pid    PID of the child to wait for, or 0 for any child
** @param status Pointer to int32 into which the child's status is placed,
**               or NULL
**
** @returns The PID of the terminated child, or an error code
**
** If there are no children in the system, returns an error code (*status
** is unchanged).
**
** If there are one or more children in the system and at least one has
** terminated but hasn't yet been cleaned up, cleans up that process and
** returns its information; otherwise, blocks until a child terminates.
*/
int32 wait( Pid pid, int32 *status );

/*
** spawn - create a new process
**
** usage:	pid = spawn(entry,args);
**
** @param entry The function which is the entry point of the new code
** @param args  An argv-style array of char *, NULL-terminated
**
** @returns PID of the new process, or an error code
*/
int32 spawn( int (*entry)(int,char*), char *args[] );

/*
** read - read into a buffer from a stream
**
** usage:	n = read(channel,buf,length)
**
** @param chan   I/O stream to read from
** @param buf    Buffer to read into
** @param length Maximum capacity of the buffer
**
** @returns      The count of bytes transferred, or an error code
*/
int32 read( int chan, void *buffer, uint32 length );

/*
** write - write from a buffer to a stream
**
** usage:	n = write(channel,buf,size)
**
** @param chan   I/O stream to write to
** @param buf    Buffer to write from
** @param size   Number of bytes to write
**
** @returns      The count of bytes transferred, or an error code
*/
int32 write( int chan, const void *buf, uint32 length );

/*
** sleep - put the current process to sleep for some length of time
**
** usage:	sleep(n);
**
** @param n Desired sleep time, in MS, or 0 to yield the CPU
*/
void sleep( uint32 msec );

/*
** gettime - retrieve the current system time
**
** usage:	n = gettime();
**
** @returns The current system time
*/
Time gettime( void );

/*
** getpid - retrieve PID of this process
**
** usage:	n = getpid();
**
** @returns The PID of this process
*/
Pid getpid( void );

/*
** getppid - retrieve PID of the parent of this process
**
** usage:	n = getppid();
**
** @returns The PID of the parent of this process
*/
Pid getppid( void );

/*
** getstate - retrieve the priority of the specified process
**
** usage:	state = getstate(pid);
**
** @param pid The PID of the process to check, or 0 for this process
**
** @returns The state of the process, or an error code
*/
State getstate( uint16 pid );

/*
** bogus - a bogus system call, for testing our syscall ISR
**
** usage:	bogus();
*/
void bogus( void );

/*
**********************************************
** CONVENIENT "SHORTHAND" VERSIONS OF SYSCALLS
**********************************************
*/

/*
** cwritech(ch) - write a single character to the console
**
** @param ch The character to write
**
** @returns The return value from calling write()
*/
int cwritech( char ch );

/*
** cwrites(str) - write a NUL-terminated string to the console
**
** @param str The string to write
**
** @returns The return value from calling write()
*/
int cwrites( const char *str );

/*
** cwrite(buf,size) - write a sized buffer to the console
**
** @param buf  The buffer to write
** @param size The number of bytes to write
**
** @returns The return value from calling write()
*/
int cwrite( const char *buf, uint32 size );

/*
** swritech(ch) - write a single character to the SIO
**
** @param ch The character to write
**
** @returns The return value from calling write()
*/
int swritech( char ch );

/*
** swrites(str) - write a NUL-terminated string to the SIO
**
** @param str The string to write
**
** @returns The return value from calling write()
*/
int swrites( const char *str );

/*
** swrite(buf,size) - write a sized buffer to the SIO
**
** @param buf  The buffer to write
** @param size The number of bytes to write
**
** @returns The return value from calling write()
*/
int swrite( const char *buf, uint32 size );

/*
**********************************************
** STRING MANIPULATION FUNCTIONS
**********************************************
*/

/*
** strstate(n) - return a constant string containing a state name
**
** @param n   The string to examine
**
** @returns A pointer to the string containing the state name, or NULL
*/
const char *strstate( State n );

/*
** str2int(str,base) - convert a string to a number in the specified base
**
** @param str   The string to examine
** @param base  The radix to use in the conversion
**
** @returns The converted integer
*/
int str2int( register const char *str, register int base );

/*
** strlen(str) - return length of a NUL-terminated string
**
** @param str The string to examine
**
** @returns The length of the string, or 0
*/
uint32 strlen( const char *str );

/*
** strcpy(dst,src) - copy a NUL-terminated string
**
** @param dst The destination buffer
** @param src The source buffer
**
** @returns The dst parameter
**
** NOTE:  assumes dst is large enough to hold the copied string
*/
char *strcpy( register char *dst, register const char *src );

/*
** strcat(dst,src) - append one string to another
**
** @param dst The destination buffer
** @param src The source buffer
**
** @returns The dst parameter
**
** NOTE:  assumes dst is large enough to hold the resulting string
*/
char *strcat( register char *dst, register const char *src );

/*
** strcmp(s1,s2) - compare two NUL-terminated strings
**
** @param s1 The first source string
** @param s1 The second source string
**
** @returns negative if s1 < s2, zero if equal, and positive if s1 > s2
*/
int strcmp( register const char *s1, register const char *s2 );

/*
** pad(dst,extra,padchar) - generate a padding string
**
** @param dst     Pointer to where the padding should begin
** @param extra   How many padding bytes to add
** @param padchar What character to pad with
**
** @returns Pointer to the first byte after the padding
**
** NOTE: does NOT NUL-terminate the buffer
*/
char *pad( char *dst, int extra, int padchar );

/*
** padstr(dst,str,len,width,leftadjust,padchar - add padding characters
**                                               to a string
**
** @param dst        The destination buffer
** @param str        The string to be padded
** @param len        The string length, or -1
** @param width      The desired final length of the string
** @param leftadjust Should the string be left-justified?
** @param padchar    What character to pad with
**
** @returns Pointer to the first byte after the padded string
**
** NOTE: does NOT NUL-terminate the buffer
*/
char *padstr( char *dst, char *str, int len, int width,
                   int leftadjust, int padchar );

/*
** sprint(dst,fmt,...) - formatted output into a string buffer
**
** @param dst The string buffer
** @param fmt Format string
**
** The format string parameter is followed by zero or more additional
** parameters which are interpreted according to the format string.
**
** NOTE:  assumes the buffer is large enough to hold the result string
**
** NOTE:  relies heavily on the x86 parameter passing convention
** (parameters are pushed onto the stack in reverse order as
** 32-bit values).
*/
void sprint( char *dst, char *fmt, ... );

/* 
** Writes the character c to stream.
**
** @param c	The character to write
** @param chan	The channel to write the character to
**
** @return The character written on success, -1 on error.
*/
int fputc(char c, int chan);

/* 
** Writes the null terminated string s to stream.
**
** @param s	A pointer to the null terimnated string to write. 
** @param chan	The channel to write the character to
**
** @return The number of characters written.
*/
int fputs(const char* s, int chan);

/* 
** Reads the next character from a channel, cast the result to an int.
**
** @param chan	The channel to read the character from
**
** @return The character read cast to an int on success, -1 on error.
*/
int fgetc(int chan);

/* 
** Reads in at most one less than size characters from stream and
** stores them into the buffer pointed to by s.  Reading  stops  after  an
** EOF  or a newline.  If a newline is read, it is stored into the buffer.
** A terminating null byte ('\0') is stored after the  last  character  in
** the buffer.
**
** @param s	The buffer to read the string into
** @param size	The maximum number of characters to read
** @param chan	The channel to read the character from
**
** @return s on success, and NULL on error or EOF.
*/
char* fgets(char* s, int size, int chan);

/*
**********************************************
** MISCELLANEOUS USEFUL SUPPORT FUNCTIONS
**********************************************
*/

/*
** exit_helper()
**
** calls exit(%eax) - serves as the "return to" code for main()
** functions, in case they don't call exit() themselves
*/
void exit_helper( void );

/*
** parse_args(argc,args,n,argv)
**
** parse a command-line argument string into an argument vector
**
** @param argc    Count of arguments expected in the string
** @param args    The command-line argument string
** @param n       Length of the argv array
** @param argv    The argv array
**
** @returns The number of argument strings put into the argv array
**
** Takes the argc and args parameters to the process' main() function
** along with an array of char *; fills in the array with pointers to
** the beginnings of the argument strings, followed by a NULL pointer.
** Only converts the first n-1 entries.
*/
int parse_args( int argc, char *args, int n, char *argv[] );

/*
** cvt_dec(buf,value)
**
** convert a 32-bit signed value into a NUL-terminated character string
**
** @param buf    Destination buffer
** @param value  Value to convert
**
** @returns The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvt_dec( char *buf, int32 value );

/*
** cvt_hex(buf,value)
**
** convert a 32-bit unsigned value into an (up to) 8-character
** NUL-terminated character string
**
** @param buf    Destination buffer
** @param value  Value to convert
**
** @returns The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvt_hex( char *buf, uint32 value );

/*
** cvt_oct(buf,value)
**
** convert a 32-bit unsigned value into an (up to) 11-character
** NUL-terminated character string
**
** @param buf   Destination buffer
** @param value Value to convert
**
** @returns The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
int cvt_oct( char *buf, uint32 value );

/*
** report(ch,pid) - report to the console that user 'ch' is running as 'pid'
**
** @param ch   The one-character name of the user process
** @param whom The PID of the process
*/
void report( char ch, Pid whom );

/*
** parse_args(argc,args,n,argv) - argument parser.
**
** Takes the argc and args parameters to the process' main() function
** along with an array of char *; fills in the array with pointers to the
** beginnings of the argument strings, followed by a NULL pointer.
**
** Only converts the first n-1 entries.
**
** @param argc  Number of strings expected in 'args'
** @param args  Argument string
** @param n     Length of 'argv' array
** @param argv  Array of char* into which string pointers are placed
**
** @returns number of strings that were placed into argv
*/
int parse_args( int argc, char *args, int n, char *argv[] );

#endif

#endif
