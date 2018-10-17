# Makefile for Square client and server
#
EXE 	=	SquareClient SquareServer 
SRC 	=	client/SquareClient.c server/SquareServer.c
FLAGS	=	-Wall -g -DTRACE

all:		$(SRC) errlib.h errlib.c  sockwrap.h sockwrap.c types.o
		cc $(FLAGS) -o SquareClient client/*.c errlib.c sockwrap.c types.o -I client -lpthread -lm
		cc $(FLAGS) -o SquareServer server/*.c errlib.c sockwrap.c types.o -I server -lpthread -lm

types.h:	types.xdr 
		rpcgen -h -o types.h types.xdr

types.c:	types.xdr 
		rpcgen -c -o types.c types.xdr

types.o:	types.c types.h
		cc -c -o types.o types.c

