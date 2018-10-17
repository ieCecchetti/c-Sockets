/*
 * 	File SquareServer.c
 *	A UDP server that receives a sequence of hyper XDR integers and echoes its square
 *      - Gets port from command line, default is 2050.
 *      - SEQUENTIAL server (serves one request at a time)
 */


#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <inttypes.h>
#include    <rpc/xdr.h>
#include    "types.h"
#include    "errlib.h"
#include    "sockwrap.h"

#define BUFLEN		65536
#define DEFAULT_PORT	2050

/* FUNCTION PROTOTYPES */
int udp_server_init(uint16_t port);
int service (char * buf, int n, struct sockaddr_in *from, int s);

/* GLOBAL VARIABLES */
char *prog_name;

int main(int argc, char *argv[])
{
	char	 		buf[BUFLEN];		/* reception buffer */
	uint16_t 		lport_n, lport_h;	/* port where server listens */
	int			s;			/* socket where server listens */
	int			n;			/* received datagram length or error code */
	struct sockaddr_in	from;			/* client address */
	socklen_t 		addrlen;		/* client address length */

	prog_name = argv[0];

	/* get server port number */
	lport_h = DEFAULT_PORT;
	if (argc > 2)
	    if (sscanf(argv[1], "%" SCNu16, &lport_h)!=1) {
		printf("Invalid port number, using default\n");
		lport_h = DEFAULT_PORT;
	    }
  	lport_n = htons(lport_h);

	/* initialize server UDP socket */
	s = udp_server_init(lport_n);

	/* main server loop */
	for (;;)
	{
	    addrlen = sizeof(struct sockaddr_in);
	    n=recvfrom(s, buf, BUFLEN-1, 0, (struct sockaddr *)&from, &addrlen);
	    if (n != -1) {
		if (service(buf, n, &from, s)==-1)
			printf("Service performed with error\n");
		else
			printf("Service performed successfully\n");
	   }
	}
}

/* Creates a UDP socket and binds it to the specified port and all
   local IP addresses
*/
int udp_server_init(uint16_t port)
{
	int			s;	/* socket where server listens */
	int 			result;	/* error code */
	struct sockaddr_in	saddr;	/* server address */

	/* create the socket */
	printf("creating socket\n");
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1)
		err_sys("socket() failed");
	printf("done, socket number %u\n",s);

	/* bind the socket to all local IP addresses */
	saddr.sin_family      = AF_INET;
	saddr.sin_port        = port;
	saddr.sin_addr.s_addr = INADDR_ANY;
	showAddr("Binding to address", &saddr);
	result = bind(s, (struct sockaddr *) &saddr, sizeof(saddr));
	if (result == -1)
		err_sys("bind() failed");
	printf("done.\n");
	return s;
}

/* Decodes the request, computes the response and sends it back.
   Returns 0 on success, -1 on error
*/
int service (char *buf, int n, struct sockaddr_in *from, int s)
{
	XDR 		xdrs_in;	/* input XDR stream */
	XDR		xdrs_out;	/* output XDR stream */
	xdrhypersequence req;		/* request message */
	Response 	res;		/* response message */
	char	 	obuf[BUFLEN];	/* output buffer */
	unsigned int 	len,i;		/* length of output message */


	/* initialize XDR input stream to point to buf */
	xdrmem_create(&xdrs_in, buf, n, XDR_DECODE);

	/* decode request */
	req.xdrhypersequence_len = 100;
	req.xdrhypersequence_val = (xdrhyper *)malloc(100*sizeof(xdrhyper));
	if (req.xdrhypersequence_val==NULL){
	    xdr_destroy(&xdrs_in);
	    return -1;
	}
	if (!xdr_xdrhypersequence(&xdrs_in, &req)) {
	    xdr_destroy(&xdrs_in);
	    return -1;
	}
	showAddr("Received message from", from);
	printf("%d bytes\n", n);

	/* compute square and fill response structure */
	res.request.xdrhypersequence_val = (xdrhyper *)malloc(req.xdrhypersequence_len*sizeof(xdrhyper));
	res.request.xdrhypersequence_len = req.xdrhypersequence_len;
	res.response.xdrhypersequence_val = (xdrhyper *)malloc(req.xdrhypersequence_len*sizeof(xdrhyper));
	res.response.xdrhypersequence_len = req.xdrhypersequence_len;
    if (res.request.xdrhypersequence_val==NULL || res.response.xdrhypersequence_val==NULL){
	    xdr_destroy(&xdrs_in);
	    return -1;
	}
    for (i=0; i<req.xdrhypersequence_len; i++) {
	    res.request.xdrhypersequence_val[i] = req.xdrhypersequence_val[i];
	    res.response.xdrhypersequence_val[i] = res.request.xdrhypersequence_val[i] * res.request.xdrhypersequence_val[i];
	}

	/* initialize XDR output stream to point to obuf */
	xdrmem_create(&xdrs_out, obuf, BUFLEN, XDR_ENCODE);

	/* encode response structure */
	if (!xdr_Response(&xdrs_out, &res)) {
		printf("Encoding error\n");
		xdr_destroy(&xdrs_out);
		xdr_destroy(&xdrs_in);
		return -1;
	}

	/* compute length of encoded response and send it */
	len = xdr_getpos(&xdrs_out);
	if(sendto(s, obuf, len, 0, (struct sockaddr *)from, sizeof(*from)) != len) {
		printf("Write error while replying\n");
		xdr_destroy(&xdrs_out);
		xdr_destroy(&xdrs_in);
		return -1;
	}
	else {
		printf("Reply sent\n");
		xdr_destroy(&xdrs_out);
		xdr_destroy(&xdrs_in);
		return 0;
	}
}

