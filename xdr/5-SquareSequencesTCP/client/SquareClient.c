/*
 *	File SquareClient.c
 *      A TCP client for the Square service
 *      - Gets server IP address and port from command line, default is 127.0.0.1!2050.
 *        > continuously reads pairs of integers from keyboard
 *        > for each read pair, the client 
 *          - connects to the server
 *          - sends the integer pair to the server as a XDR hyper integer sequence of length 2
 *          - waits for response and diaplays result
 *          - no timeout on waiting for response
 *	
 */


#include     <stdio.h>
#include     <stdlib.h>
#include     <string.h>
#include     <inttypes.h>
#include     <rpc/xdr.h>
#include     <sys/types.h>
#include     <sys/socket.h>
#include     "../errlib.h"
#include     "../sockwrap.h"
#include     "../types.h"

#define BUFLEN 512
#define DEFAULT_PORT	2050
#define DEFAULT_ADDR	"127.0.0.1"

char* prog_name;

int main(int argc, char *argv[])
{
	char     		buf[BUFLEN];	   /* transmission buffer */
	char	 		rbuf[BUFLEN];	   /* reception buffer */

	uint16_t		tport_n, tport_h;  /* server port number */

	int			s;		   /* socket used for communication with the server */
	struct in_addr		sinaddr_def;	   /* server IP address structure (default value) */
	struct in_addr		sinaddr;	   /* server IP address structure */
	struct sockaddr_in	saddr;  	   /* server address */

	prog_name = argv[0];

	/* get port of server */
	if (argc < 3) {
		printf("Using default port.\n");
		tport_h = DEFAULT_PORT;
	} else if (sscanf(argv[2], "%" SCNu16, &tport_h)!=1) {
		printf("Invalid port number. Using default.\n"); 
		tport_h = DEFAULT_PORT;
	}
	tport_n = htons(tport_h);


	/* get IP of server */
	if (!inet_aton(DEFAULT_ADDR, &sinaddr_def))
		err_quit("Internal error while converting default IP address");
	if (argc < 2) {
		printf("Using default IP.\n");
		sinaddr = sinaddr_def;
	} else if (!inet_aton(argv[1], &sinaddr)) {
		printf("Invalid IP address. Using default\n");
		sinaddr = sinaddr_def;
	}
	
	/* main client loop */	
	for (;;)
	{
	    int			len, n;
	    unsigned int 	input1, input2, output1, output2;
	    xdrhypersequence	req;		/* request data */
	    Response		res;		/* response data */
	    XDR 		xdrs_in;	/* input XDR stream */
	    XDR			xdrs_out;	/* output XDR stream */
	    xdrhyper		inseq[2];
	    xdrhyper		outseq[2];

	    /* read 2 integers from keyboard */
	    printf(" Enter integer: ");
	    fgets(buf, BUFLEN, stdin);
	    sscanf(buf, "%u", &input1);
	    printf(" Enter integer: ");
	    fgets(buf, BUFLEN, stdin);
	    sscanf(buf, "%u", &input2);

	    /* initialize client TCP socket */
	    s = Socket (AF_INET, SOCK_STREAM, 0);

	    /* prepare server address structure */
    	    saddr.sin_family      = AF_INET;
	    saddr.sin_port        = tport_n;
	    saddr.sin_addr 	      = sinaddr;
	
	    /* connect to server */
	    Connect (s, (struct sockaddr*) &saddr, sizeof(saddr));

	    /* create requerst (local representation ) */
	    req.xdrhypersequence_len = 2;
	    req.xdrhypersequence_val = outseq;
	    req.xdrhypersequence_val[0] = input1;
	    req.xdrhypersequence_val[1] = input2;

	    /* encode request */
	    xdrmem_create(&xdrs_out, buf, BUFLEN, XDR_ENCODE);
	    if (!xdr_xdrhypersequence(&xdrs_out, &req)) {
	        xdr_destroy(&xdrs_out);
	        return -1;
	    }

	    /* send request */
	    len = xdr_getpos(&xdrs_out);
	    Writen(s, buf, len);
	    xdr_destroy(&xdrs_out);

	    /* receive response */
	    printf("Waiting for response...\n");
	    n=Readn(s,rbuf,2*(4+8*2));
	    printf("Received response (%d bytes)\n", n);
            if (n == 2*(4+8*2))
	    {
		/* response received of right length. Decode it */
		xdrmem_create(&xdrs_in, rbuf, n, XDR_DECODE);
		res.request.xdrhypersequence_len=2;
		res.request.xdrhypersequence_val=inseq;
		res.response.xdrhypersequence_len=2;
		res.response.xdrhypersequence_val=outseq;

		if (!xdr_Response(&xdrs_in, &res)) {
			printf("Error in decoding response\n");
		} else {
			input1 = res.request.xdrhypersequence_len;
			output1 = res.response.xdrhypersequence_len;
			printf("Request size: %u\n", input1);
			printf("Result size: %u\n", output1);
			input1 = res.request.xdrhypersequence_val[0];
			output1 = res.response.xdrhypersequence_val[0];
			printf("Request value 1: %u\n", input1);
			printf("Result value 1: %u\n", output1);
			input2 = res.request.xdrhypersequence_val[1];
			output2 = res.response.xdrhypersequence_val[1];
			printf("Request value 2: %u\n", input2);
			printf("Result value 2: %u\n", output2);
		}
		xdr_destroy(&xdrs_in);			
	    }
	    else printf("Error in receiving response\n");
	    printf("=======================================================\n");

	    /* close socket */
	    Close(s);
	}
	exit(0);
}

