Simple application using XDR data types (computing the square of a sequence of 8-bytes integers).
The application uses the TCP protocol.
The sequences can have up to 2^32-1 integers.
The implementation is for the Unix systems and is based on the library functions in the book by Stevens.

types.xdr	XDR type definitions
server		server side
client		client side
sockwrap.h	header for the common functions
sockwrap.c	function implementation for the common functions
errlib.h	header for the error management functions
errlib.c	function implementation for error management functions
makefile	makefile
readme.txt	this file

The following steps are necessary for testing the application:
1. run the make command for compiling:
$ make
2. run the server in one window:
$ ./SquareServer
3. and the client in another wondow:
$ ./SquareClient

