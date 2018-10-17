/*
 * 	File SquareServer.c
 *	A TCP server that receives a sequence of hyper XDR integers and echoes its square
 *      - Gets port from command line, default is 2050.
 *      - CONCURRENT server with process pool for Unix-like systems
 *	- this version does not use the buffer paradigm but connects local representations
 *	  to sockets directly
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <rpc/xdr.h>

#include "../errlib.h"
#include "../sockwrap.h"
#include "../types.h"

#define DEFAULT_PORT	2050
#define LISTENQ 15			/* backlog */

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

#define CHILDREN	5		/* the number of processes in the pool */


/* FUNCTION PROTOTYPES */
int service (int s);
void child(int sock);

char *prog_name;			/* the name of the program (for logging) */

int main(int argc, char *argv[])
{
	uint16_t 		lport_n, lport_h;	/* port where server listens */
	int			s;			/* socket where server listens */
	struct sockaddr_in 	servaddr;		/* server address */
	int k=0;
	int process_id;
	int pid[CHILDREN];

	/* for errlib to know the program name */
	prog_name = argv[0];

	/* get server port number */
	lport_h = DEFAULT_PORT;
	if (argc > 1)
	    if (sscanf(argv[1], "%" SCNu16, &lport_h)!=1) {
		printf("Invalid port number, using default\n");
		lport_h = DEFAULT_PORT;
	    }
  	lport_n = htons(lport_h);

	/* initialize server TCP socket */
	s = Socket(AF_INET, SOCK_STREAM, 0);

	/* Bind the socket */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = lport_n;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	Bind(s, (SA*) &servaddr, sizeof(servaddr));

	trace ( err_msg("(%s) socket created",prog_name) );

	/* Start listening */
	trace ( err_msg("(%s) listening on %s:%u", prog_name, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port)) );

	Listen(s, LISTENQ);

	/* Create process pool */
	for(k=0; k<CHILDREN; k++)
	{
		process_id = fork();
		if(process_id < 0)
		{
			err_sys("Cannot fork. Exiting\n");
		}
		else if(process_id == 0)
		{
			/* Child */
			child(s);
			return 0;
		}
		else
		{
			/* Parent. Store the child's pid */
			pid[k] = process_id;
		}
	}

	/* wait for children termination in order to avoid zombies */
	for(k=0; k<CHILDREN; k++)
		waitpid(pid[k], NULL, 0);
	return 0;
}


void child(int sock)
{
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);
	int connfd;

	/* main server loop */
	for (;;)
	{
		trace( err_msg ("(%s) waiting for connections ...", prog_name) );

		connfd = Accept (sock, (SA*) &cliaddr, &cliaddrlen);
		trace ( err_msg("(%s) - new connection from client %s:%u", prog_name, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port)) );

		if (service(connfd)==-1)
			trace ( err_msg("(%s) - Service performed with error", prog_name) );
		else
			trace ( err_msg("(%s) - Service performed successfully", prog_name) );
		Close(connfd);

		trace( err_msg ("(%s) - connection closed", prog_name) );
	}
	return;
}

/* Decodes the request, computes the response and sends it back.
   Returns 0 on success, -1 on error
*/
int service (int s)
{
	XDR 		xdrs_in;	/* input XDR stream */
	XDR		xdrs_out;	/* output XDR stream */
	xdrhypersequence req;		/* request message */
	Response 	res;		/* response message */
	unsigned int 	i;
	FILE *stream_socket_r;		/* FILE stream for reading from the socket */
	FILE *stream_socket_w;		/* FILE stream for writing to the socket */

	/* open FILE streams for the socket anFILEd bind them to corresponding xdr streams */
	stream_socket_r = fdopen(s, "r");
	if (stream_socket_r == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_in, stream_socket_r, XDR_DECODE);

	stream_socket_w = fdopen(s, "w");
	if (stream_socket_w == NULL)
		err_sys ("(%s) error - fdopen() failed", prog_name);
	xdrstdio_create(&xdrs_out, stream_socket_w, XDR_ENCODE);

	trace( err_msg("(%s) - waiting for operands ...", prog_name) );

	/* receive request */
	req.xdrhypersequence_len = 0;
	req.xdrhypersequence_val = NULL;	/* xdr library will allocate memory */

	if (!xdr_xdrhypersequence(&xdrs_in, &req)) {
	    xdr_destroy(&xdrs_in);
	    return -1;
	}
	printf("Received request\n");

	/* compute square and fill response structure */
	res.request.xdrhypersequence_val = (xdrhyper *)malloc(req.xdrhypersequence_len*sizeof(xdrhyper));
        if (res.request.xdrhypersequence_val==NULL){
	    free(req.xdrhypersequence_val);
	    xdr_destroy(&xdrs_in);
	    return -1;
	}
	res.request.xdrhypersequence_len = req.xdrhypersequence_len;
	res.response.xdrhypersequence_val = (xdrhyper *)malloc(req.xdrhypersequence_len*sizeof(xdrhyper));
        if (res.response.xdrhypersequence_val==NULL){
	    xdr_destroy(&xdrs_in);
	    free(req.xdrhypersequence_val);
	    free(res.request.xdrhypersequence_val);
	    return -1;
	}
	res.response.xdrhypersequence_len = req.xdrhypersequence_len;
	for (i=0; i<req.xdrhypersequence_len; i++) {
	    res.request.xdrhypersequence_val[i] = req.xdrhypersequence_val[i];
	    res.response.xdrhypersequence_val[i] = res.request.xdrhypersequence_val[i] * res.request.xdrhypersequence_val[i];
	}

	/* encode and send response structure */
	if (!xdr_Response(&xdrs_out, &res)) {
		printf("Encoding error\n");
		xdr_destroy(&xdrs_out);
		xdr_destroy(&xdrs_in);
		free(req.xdrhypersequence_val);
	    	free(res.request.xdrhypersequence_val);
	    	free(res.response.xdrhypersequence_val);
		return -1;
	}
	fflush(stream_socket_w);

	free(req.xdrhypersequence_val);
	free(res.request.xdrhypersequence_val);
    	free(res.response.xdrhypersequence_val);
	return 0;
}

