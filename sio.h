/*
** SCCS ID:	%W%	%G%
**
** File:	sio.h
**
** Author:	Warren R. Carithers
**
** Contributor:
**
** Description:	SIO definitions
*/

#ifndef _SIO_H_
#define _SIO_H_

/*
** General (C and/or assembly) definitions
*/

// sio interrupt settings

#define	SIO_TX		0x01
#define	SIO_RX		0x02
#define	SIO_BOTH	(SIO_TX | SIO_RX)

#ifndef __SP_ASM__

#include "common.h"
#include "queues.h"

/*
** Start of C-only definitions
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _sio_init()
**
** Initialize the UART chip.
*/
void _sio_init( void );

/*
** _sio_enable()
**
** enable/disable SIO interrupts
**
** usage:       uint8 old = _sio_enable( uint8 which )
**
** enables interrupts according to the 'which' parameter
**
** returns the prior settings
*/
uint8 _sio_enable( uint8 which );

/*
** _sio_disable()
**
** disable/disable SIO interrupts
**
** usage:       uint8 old = _sio_disable( uint8 which )
**
** disables interrupts according to the 'which' parameter
**
** returns the prior settings
*/
uint8 _sio_disable( uint8 which );

/*
** _sio_input_queue()
**
** get the input queue length
**
** usage:	int num = _sio_input_queue()
**
** returns the count of characters still in the input queue
*/
int _sio_input_queue( void );

/*
** _sio_readc()
**
** get the next input character
**
** usage:	int ch = _sio_readc()
**
** returns the next character, or -1 if no character is available
*/
int _sio_readc( void );

/*
** _sio_reads()
**
** get an input line, up to a specific number of characters
**
** usage:	int num = _sio_reads( char *buffer, length)
**
** returns the number of characters put into the buffer, or 0 if no
** characters are available
*/
int _sio_reads( char *buffer, int length );

/*
** _sio_writec( ch )
**
** write a character to the serial output
**
** usage:	_sio_writec( int ch )
*/
void _sio_writec( int ch );

/*
** _sio_write( ch )
**
** write a buffer of characters to the serial output
**
** usage:	int num = _sio_write( const char *buffer, int length )
**
** returns the count of bytes transferred
*/
int _sio_write( const char *buffer, int length );

/*
** _sio_puts( buf )
**
** write a NUL-terminated buffer of characters to the serial output
**
** usage:	n = _sio_puts( const char *buffer );
**
** returns the count of bytes transferred
*/
int _sio_puts( const char *buffer );

/*
** _sio_dump( full )
**
** dump the contents of the SIO buffers
*/
void _sio_dump( bool full );

#endif

#endif
