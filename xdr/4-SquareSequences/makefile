# Makefile for Square client and server
#
EXE 	=	SquareClient SquareServer 
OBJ 	=	SquareClient.o SquareServer.o 
SRC 	=	SquareClient.c SquareServer.c 
FLAGS	=	-Wall

all:		$(OBJ) sockwrap.o errlib.o types.o
		for f in $(EXE); do (cc -o $$f $$f.o sockwrap.o errlib.o types.o); done

SquareClient.o:	SquareClient.c types.h types.c
		cc $(FLAGS) -c -o SquareClient.o SquareClient.c

SquareServer.o:	SquareServer.c types.h types.c
		cc $(FLAGS) -c -o SquareServer.o SquareServer.c

sockwrap.o:	sockwrap.c sockwrap.h
		cc $(FLAGS) -c -o sockwrap.o sockwrap.c

errlib.o:	errlib.c errlib.h
		cc $(FLAGS) -c -o errlib.o errlib.c

types.o:	types.c types.h
		cc -c -o types.o types.c

types.h:	types.xdr 
		rpcgen -h -o types.h types.xdr

types.c:	types.xdr 
		rpcgen -c -o types.c types.xdr
