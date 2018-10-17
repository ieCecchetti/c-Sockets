/*
 *	File Client1u.C
 *      ECHO UDP CLIENT with the following feqatures:
 *      - Gets server IP address and port from keyboard 
 *      - LINE/ORIENTED:
 *	  > sends a ACK mess to server that reply with a certain sentence
 *	  > exit if no reply after 15 sec
 *        > after the init sentence continuously reads lines from keyboard
 *        > sends each line to the server
 *      - Terminates when the "close" or "stop" line is entered
 */


#include     <stdlib.h>
#include     <string.h>
#include     <inttypes.h>
#include     "../errlib.h"
#include     "../sockwrap.h"

#define BUFLEN 128  		/* BUFFER LENGTH */
#define TIMEOUT 15  		/* TIMEOUT (seconds) */
char    buf[BUFLEN];	    /* transmission buffer */
char	rbuf[BUFLEN];	    /* reception buffer */

/* FUNCTION PROTOTYPES */
int mygetline(char *line, size_t maxline, char *prompt);
void client_service(int socket, struct sockaddr_in	saddr);

/* GLOBAL VARIABLES */
char *prog_name;


int main(int argc, char *argv[])
{
    uint32_t		taddr_n;  /* server IP addr. (net/host ord) */
    uint16_t		tport_n, tport_h;  /* server port number */

    int		s;
    struct sockaddr_in	saddr;

    prog_name = argv[0];

	/* argc contains the main arguments so the caller parameters...
     * lets store 'em, but before, we check if the minimum of them are reached */    
    if(argc < 3)
    {
        printf("Usage: %s <server_addr> <port number>\n", prog_name); 
        exit(1);
    } 

    /* input IP address and port of server */
    taddr_n = inet_addr(argv[1]);
    if (taddr_n == INADDR_NONE)
	err_sys("Invalid address");

    if (sscanf(argv[2], "%" SCNu16, &tport_h)!=1)
	err_sys("Invalid port number");
    tport_n = htons(tport_h);

    /* create the socket */
    printf("Creating socket\n");
    s = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printf("done. Socket number: %d\n",s);

    /* prepare server address structure */
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family      = AF_INET;
    saddr.sin_port        = tport_n;
    saddr.sin_addr.s_addr = taddr_n;

	/* main client loop */
    client_service(s, saddr);
    
	close(s);
	exit(0);
}

/* Gets a line of text from standard input after having printed a prompt string 
   Substitutes end of line with '\0'
   Empties standard input buffer but stores at most maxline-1 characters in the
   passed buffer
*/
int mygetline(char *line, size_t maxline, char *prompt)
{
	char	ch;
	size_t 	i;

	printf("%s", prompt);
	for (i=0; i< maxline-1 && (ch = getchar()) != '\n' && ch != EOF; i++)
		*line++ = ch;
	*line = '\0';
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	if (ch == EOF)
		return(EOF);
	else    return(1);
}

void client_service(int socket, struct sockaddr_in	saddr)
{	
	size_t				len, n;
	struct sockaddr_in 	from;
	socklen_t 			fromlen;
	
	fd_set		cset;
    struct timeval	tval;
	    
    strcpy(buf,"\0");
    
    printf("sending the ACK message to the server -> ");
    strcpy(buf, "ACK");
	len = strlen(buf);
	if (sendto(socket, buf, len, 0, (struct sockaddr *) &saddr, sizeof(saddr)) != len)
	{
		printf("Write error\n");
	}else
	{
		printf("Done\n");
	}
    
	FD_ZERO(&cset);
	FD_SET(socket, &cset);
	tval.tv_sec = TIMEOUT;
	tval.tv_usec = 0;
	n = Select(FD_SETSIZE, &cset, NULL, NULL, &tval);
	if (n>0)
	{
		/* receive datagram */
		fromlen = sizeof(struct sockaddr_in);
		n=recvfrom(socket,rbuf,BUFLEN-1,0,(struct sockaddr *)&from,&fromlen);
		if (n != -1)
		{
			rbuf[n] = '\0';
			showAddr("Received response from", &from);
			printf(": [%s]\n", rbuf);
		}
		else{
			printf("Error in receiving initialization... Server busy\n");	
			return;		
		}
	}
	else 
	{
		printf("No response received after %d seconds\n",TIMEOUT);
		return;
	}
    
    while(strcmp(buf,"CLOSE")!=0)
    {
	    mygetline(buf, BUFLEN, "Enter line (max 127 char) or write 'CLOSE' to stop: ");
	    len = strlen(buf);
	    n=sendto(socket, buf, len, 0, (struct sockaddr *) &saddr, sizeof(saddr));
	    if (n != len)
	    {
			printf("Write error\n");
			continue;
	    }
	    printf("=======================================================\n");
	}

}
	
