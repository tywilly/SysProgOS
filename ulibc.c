/*
** SCCS ID:	@(#)ulibc.c	1.1	3/30/20
**
** File:	ulibc.c
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	C implementations of user-level library functions
*/

#include "common.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

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
int32 cwritech( char ch ) {
   return( write(CHAN_CONS,&ch,1) );
}

/*
** cwrites(str) - write a NUL-terminated string to the console
**
** @param str The string to write
**
*/
int32 cwrites( const char *str ) {
   int len = strlen(str);
   return( write(CHAN_CONS,str,len) );
}

/*
** cwrite(buf,size) - write a sized buffer to the console
**
** @param buf  The buffer to write
** @param size The number of bytes to write
**
** @returns The return value from calling write()
*/
int32 cwrite( const char *buf, uint32 size ) {
   return( write(CHAN_CONS,buf,size) );
}

/*
** swritech(ch) - write a single character to the SIO
**
** @param ch The character to write
**
** @returns The return value from calling write()
*/
int32 swritech( char ch ) {
   return( write(CHAN_SIO,&ch,1) );
}

/*
** swrites(str) - write a NUL-terminated string to the SIO
**
** @param str The string to write
**
** @returns The return value from calling write()
*/
int32 swrites( const char *str ) {
   int len = strlen(str);
   return( write(CHAN_SIO,str,len) );
}

/*
** swrite(buf,size) - write a sized buffer to the SIO
**
** @param buf  The buffer to write
** @param size The number of bytes to write
**
** @returns The return value from calling write()
*/
int32 swrite( const char *buf, uint32 size ) {
   return( write(CHAN_SIO,buf,size) );
}

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
** @returns A pointer to the string containing the state name
**
** N.B.:  Must be updated if the set of states changes!
*/
const char *strstate( State n ) {

    switch( n ) {
    case UNUSED:   return( "Unused" );
    case NEW:      return( "New" );
    case READY:    return( "Ready" );
    case RUNNING:  return( "Running" );
    case SLEEPING: return( "Sleeping" );
    case BLOCKED:  return( "Blocked" );
    case ZOMBIE:   return( "Zombie" );
    }

    return( "????" );
}

/*
** str2int(str,base) - convert a string to a number in the specified base
**
** @param str   The string to examine
** @param base  The radix to use in the conversion
**
** @returns The converted integer
*/
int str2int( register const char *str, register int base ) {
    register int num = 0;
    register char bchar = '9';
    int sign = 1;

    // check for leading '-'
    if( *str == '-' ) {
        sign = -1;
        ++str;
    }

    if( base != 10 ) {
        bchar = '0' + base - 1;
    }

    // iterate through the characters
    while( *str ) {
        if( *str < '0' || *str > bchar )
            break;
        num = num * base + *str - '0';
        ++str;
    }

    // return the converted value
    return( num * sign );
}

/*
** strlen(str) - return length of a NUL-terminated string
**
** @param str The string to examine
**
** @returns The length of the string, or 0
*/
uint32 strlen( register const char *str ) {
    register uint32 len = 0;

    while( *str++ ) {
        ++len;
    }

    return( len );
}

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
char *strcpy( register char *dst, register const char *src ) {
    register char *tmp = dst;

    while( (*dst++ = *src++) )
        ;

    return( tmp );
}

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
char *strcat( register char *dst, register const char *src ) {
    register char *tmp = dst;

    while( *dst )  // find the NUL
        ++dst;

    while( (*dst++ = *src++) )  // append the src string
        ;

    return( tmp );
}

/*
** strcmp(s1,s2) - compare two NUL-terminated strings
**
** @param s1 The first source string
** @param s1 The second source string
**
** @returns negative if s1 < s2, zero if equal, and positive if s1 > s2
*/
int strcmp( register const char *s1, register const char *s2 ) {

    while( *s1 != 0 && (*s1 == *s2) )
        ++s1, ++s2;

    return( *(const unsigned char *)s1 - *(const unsigned char *)s2 );
}

/*
** Splits the string s at the first occurrance of a character in the delim
** string, replacing the delimiter with a null byte.
**
** @param s	The string to parse
** @param delim	A character array of delimiters to split at
**
** @return A pointer to the beginning of the rest of the string after after the
** delimiter, or NULL if no delimiters were found.
*/
char* strsplit (char* s, const char* delim) {
	int i, j;
	int numdelim = strlen(delim);
	for (i = 0; i < strlen(s); i++) {
		for (j = 0; j < numdelim; j++) {
			if ( s[i] == delim[j] ) {
				s[i] = '\0';
				return &s[i+1];
			}
		}
	}
	return NULL;
}


/*
** pad(dst,extra,padchar) - generate a padding string
**
** @param dst     Pointer to where the padding should begin
** @param extra   How many padding bytes to add
** @param padchar What character to pad with
**
** @returns Pointer to the first byte after the padding
*/
char *pad( char *dst, int extra, int padchar ) {
    while( extra > 0 ){
        *dst++ = (char) padchar;
        extra -= 1;
    }
    return dst;
}

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
*/
char *padstr( char *dst, char *str, int len, int width,
                   int leftadjust, int padchar ) {
    int    extra;

    // determine the length of the string if we need to
    if( len < 0 ){
        len = strlen( str );
    }

    // how much filler must we add?
    extra = width - len;

    // add filler on the left if we're not left-justifying
    if( extra > 0 && !leftadjust ){
        dst = pad( dst, extra, padchar );
    }

    // copy the string itself
    for( int i = 0; i < len; ++i ) {
        *dst++ = str[i];
    }

    // add filler on the right if we are left-justifying
    if( extra > 0 && leftadjust ){
        dst = pad( dst, extra, padchar );
    }

    return dst;
}

/*
** sprint(dst,fmt,...) - formatted output into a string buffer
**
** @param dst The string buffer
** @param fmt Format string
**
** The format string parameter is followed by zero or more additional
** parameters which are interpreted according to the format string.
**
** NOTE:  relies heavily on the x86 parameter passing convention
** (parameters are pushed onto the stack in reverse order as
** 32-bit values).
*/
void sprint( char *dst, char *fmt, ... ) {
    int32 *ap;
    char buf[ 12 ];
    char ch;
    char *str;
    int leftadjust;
    int width;
    int len;
    int padchar;

    /*
    ** Get characters from the format string and process them
    **
    ** We use the "old-school" method of handling variable numbers
    ** of parameters.  We assume that parameters are passed on the
    ** runtime stack in consecutive longwords; thus, if the first
    ** parameter is at location 'x', the second is at 'x+4', the
    ** third at 'x+8', etc.  We use a pointer to a 32-bit thing
    ** to point to the next "thing", and interpret it according
    ** to the format string.
    */
    
    // get the pointer to the first "value" parameter
    ap = (int *)(&fmt) + 1;

    // iterate through the format string
    while( (ch = *fmt++) != '\0' ){
        /*
        ** Is it the start of a format code?
        */
        if( ch == '%' ){
            /*
            ** Yes, get the padding and width options (if there).
            ** Alignment must come at the beginning, then fill,
            ** then width.
            */
            leftadjust = 0;
            padchar = ' ';
            width = 0;
            ch = *fmt++;
            if( ch == '-' ){
                leftadjust = 1;
                ch = *fmt++;
            }
            if( ch == '0' ){
                padchar = '0';
                ch = *fmt++;
            }
            while( ch >= '0' && ch <= '9' ){
                width *= 10;
                width += ch - '0';
                ch = *fmt++;
            }

            /*
            ** What data type do we have?
            */
            switch( ch ) {

            case 'c':  // characters are passed as 32-bit values
                ch = *ap++;
                buf[ 0 ] = ch;
                buf[ 1 ] = '\0';
                dst = padstr( dst, buf, 1, width, leftadjust, padchar );
                break;

            case 'd':
                len = cvt_dec( buf, *ap++ );
                dst = padstr( dst, buf, len, width, leftadjust, padchar );
                break;

            case 's':
                str = (char *) (*ap++);
                dst = padstr( dst, str, -1, width, leftadjust, padchar );
                break;

            case 'x':
                len = cvt_hex( buf, *ap++ ) + 2;
                dst = padstr( dst, buf, len, width, leftadjust, padchar );
                break;

            case 'o':
                len = cvt_oct( buf, *ap++ );
                dst = padstr( dst, buf, len, width, leftadjust, padchar );
                break;

            }
        } else {
            // no, it's just an ordinary character
            *dst++ = ch;
        }
    }

    // NUL-terminate the result
    *dst = '\0';
}

/* 
** Writes the character c to stream.
**
** @param c	The character to write
** @param chan	The channel to write the character to
**
** @return The character written on success, -1 on error.
*/
int fputc(char c, int chan) {
	return write(chan, &c, 1);
}

/* 
** Writes the null terminated string s to stream.
**
** @param s	A pointer to the null terimnated string to write. 
** @param chan	The channel to write the character to
**
** @return The number of characters written.
*/
int fputs(const char* s, int chan) {
	return write(chan, s, strlen(s));
}

/* 
** Reads the next character from a channel, cast the result to an int.
**
** @param chan	The channel to read the character from
**
** @return The character read cast to an int on success, -1 on error.
*/
int fgetc(int chan) {
	char c;
	int stat;
	stat = read(chan, &c, 1);
	if (stat != 1)
		return -1;
	return (int) c;
}

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
char* fgets(char* s, int size, int chan) {
	int i;
	int stat;
	char c;
	for (i = 0; i < size - 1; i++) {
		stat = read(chan, &c, 1);
		if (stat != 1) {
			s[i] = '\0';
			return NULL;
		}
		s[i] = c;
	}
	s[i] = '\0';
	return s;
}

/*
** Gets a line with user editing, saving it to buf and null terminating it.
** Reads until one of the following happens: size-1 characters are read, a '\n'
** is read, or EOF.
**
** @param prompt	The prompt to display at the beginning of the line. If
**			this is NULL or an empty string, no prompt will be
**			printed.
** @param buf		The buffer to save the read string to.
** @param size		The size of buffer. At most, size-1 characters will be
**			read.
** @param chanin	The channel to read input from
** @param chanout	The channel to write output to
**
** @return The number of characters read.
*/
int readline(const char* prompt, char* buf, int size, int chanin, int chanout) {
	int i;

	if (prompt != NULL && prompt[0] != '\0')
		fputs(prompt, chanout);

	for (i = 0; i < size; i++) {
		int n;
		char c; 
		n = fgetc(chanin);

		if (n == -1) {
			buf[i] = '\0';
			return i;
		}

		c = (char) n;
		switch (c) {
			case '\n':
				buf[i] = '\n';
				buf[i+1] = '\0';
				fputc('\n', chanout);
				return i;
			// Delete character (backspace on serial terminals)
			case 0x7f:
				if (i == 0) break;
				i-=2;
				fputs("\b \b", chanout);
				break;
			default:
				fputc(c, chanout);
				buf[i] = c;
		}
	}

	buf[i] = '\0';
	return i;
}


/*
**********************************************
** MISCELLANEOUS USEFUL SUPPORT FUNCTIONS
**********************************************
*/

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
int parse_args( int argc, char *args, int n, char *argv[] ) {
    register int i;
    register char *ptr = args;

    // iterate through the arguments in the string
    for( i = 0 ; i < argc && i < (n-1); ++i ) {
        // remember where the current argument begins
        argv[i] = ptr;
        // move past the trailing NUL character
        while( *ptr++ ) {
            ;
        }
    }

    // NULL pointer to terminate the argv array
    argv[i] = NULL;

    // return the converted argument count
    return( i );
}

/*
** cvt_dec0(buf,value) - local support routine for cvt_dec()
**
** convert a 32-bit unsigned integer into a NUL-terminated character string
**
** @param buf    Destination buffer
** @param value  Value to convert
**
** @returns The number of characters placed into the buffer
**          (not including the NUL)
**
** NOTE:  assumes buf is large enough to hold the resulting string
*/
char *cvt_dec0( char *buf, int value ) {
    int quotient;

    quotient = value / 10;
    if( quotient < 0 ) {
        quotient = 214748364;
        value = 8;
    }
    if( quotient != 0 ) {
        buf = cvt_dec0( buf, quotient );
    }
    *buf++ = value % 10 + '0';
    return buf;
}

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
int cvt_dec( char *buf, int32 value ) {
    char *bp = buf;

    if( value < 0 ) {
        *bp++ = '-';
        value = -value;
    }

    bp = cvt_dec0( bp, value );
    *bp  = '\0';

    return( bp - buf );
}

/*
** cvt_hex(buf,value)
**
** convert a 32-bit unsigned value into an (up to) 8-character
** NUL-terminated character string
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
int cvt_hex( char *buf, uint32 value ) {
    char hexdigits[] = "0123456789ABCDEF";
    int chars_stored = 0;

    for( int i = 0; i < 8; i += 1 ) {
        int val = value & 0xf0000000;
        if( chars_stored || val != 0 || i == 7 ) {
            ++chars_stored;
            val = (val >> 28) & 0xf;
            *buf++ = hexdigits[val];
        }
        value <<= 4;
    }

    *buf = '\0';

    return( chars_stored );
}

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
int cvt_oct( char *buf, uint32 value ){
        int     i;
        int     chars_stored = 0;
        char    *bp = buf;
        int     val;

        val = ( value & 0xc0000000 );
        val >>= 30;
        for( i = 0; i < 11; i += 1 ){

                if( i == 10 || val != 0 || chars_stored ){
                        chars_stored = 1;
                        val &= 0x7;
                        *bp++ = val + '0';
                }
                value <<= 3;
                val = ( value & 0xe0000000 );
                val >>= 29;
        }
        *bp = '\0';

        return bp - buf;
}

/*
** report(ch,pid) - report to the console that user 'ch' is running as 'pid'
**
** @param ch   The one-character name of the user process
** @param whom The PID of the process
*/
void report( char ch, Pid whom ) {
    char buf[20];  // up to " x(nnnnnnnnnnnnnnn)"

    buf[0] = ' ';
    buf[1] = ch;
    buf[2] = '(';
    int i = cvt_dec( buf+3, (int) whom );
    buf[3+i] = ')';
    buf[3+i+1] = '\0';
    cwrites( buf );  // one syscall vs. five
}
