/* Simple SSLSocket Server */
/* ssl Server realized with the openSSL lib	
 * Main Function: Server setup the socket connection and
 * start listen in the port in input. 
 * Every time a ClientSSL ask to connect Server make a fork, crating a child
 * process that will handle the exchange of messages.*/

#include    <unistd.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>
#include    <inttypes.h>
#include    <sys/stat.h>
#include    <sys/wait.h>
#include    "../errlib.h"
#include    "../sockwrap.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFLEN	128 			/* BUFFER LENGTH */
#define maxConn 1; 			    /* fix #max of connections */
char    buf[BUFLEN];			/* transmission buffer */
char	rbuf[BUFLEN];			/* reception buffer */

/* FUNCTION PROTOTYPES */
int mygetline(char * line, size_t maxline, char *prompt);
void server_service(SSL *certSSL, int socket);
void send_error(int socket);
void zombie_handler();

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile);
void InitializeSSL();
void DestroySSL();
void ShutdownSSL();

/* GLOBAL VARIABLES */
char *prog_name;
FILE *fp;
struct stat st;

int main(int argc, char *argv[])
{
    int		     conn_request_skt;		/* passive socket */
    uint16_t 	 lport_n, lport_h;		/* port used by server (net/host ord.) */
    int sockfd, newsockfd;		        	/* connected socket */
    int          bklog = maxConn;     	/* max # of connections  */
    socklen_t 	 addrlen;
    struct sockaddr_in 	saddr, caddr;	/* server and client addresses */ 
    int		     childpid;				/* pid of child process */
    
    SSL_CTX *sslctx;
	SSL *cSSL;
	
	InitializeSSL();

	/* store the prog name in the caller */
    prog_name = argv[0];

	/* check if the caller received the right # of parameters */
    if (argc != 2) {
        printf("Usage: %s <port number>\n", prog_name);
        exit(1);
    }

    /* get server port number */
    if (sscanf(argv[1], "%" SCNu16, &lport_h)!=1)
        err_sys("Invalid port number");
    lport_n = htons(lport_h);
    
    /* create the socket */ 
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    /* bind the socket to any local IP address */
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family      = AF_INET;
    saddr.sin_port        = lport_n;
    saddr.sin_addr.s_addr = INADDR_ANY;     //Server ip_adrr is 0.0.0.0
    
    /* bind */
    /* Specify the port and the local adress we are listening to */
    Bind(sockfd, (struct sockaddr *) &saddr, sizeof(saddr));

    /* listen */
    /* mark the referred socket as passive, that is a socket that will 
     * be used to accept incoming connection through accept() */
    printf("Server listening...\n");
    Listen(sockfd, bklog);
    conn_request_skt = sockfd;
        
    /* init SSL connection */    
	sslctx = SSL_CTX_new( SSLv23_server_method());
	SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
	
	/* load certs */
	LoadCertificates(sslctx, "../certificate/certificate.pem", "../certificate/key.pem");	
	
	/*
	int use_cert = SSL_CTX_use_certificate_file(sslctx, "../certificate/certificate.pem" , SSL_FILETYPE_PEM);
	int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "../certificate/certificate.pem", SSL_FILETYPE_PEM);
	*/

	cSSL = SSL_new(sslctx);
	SSL_set_fd(cSSL, newsockfd );
	//Here is the SSL Accept portion.  Now all reads and writes must use SSL
	int ssl_err = SSL_accept(cSSL);
	if(ssl_err <= 0)
	{
		//Error occurred, log and close down ssl
		printf("Error: Error in SSL connection, connection Refused!");
		ShutdownSSL();
		exit(0);
	}  
	printf("All ready for ssl connection...\n");  
    
    /* main server loop */
    for (;;)
    {
		/* accept next connection */
        addrlen = sizeof(struct sockaddr_in);
        sockfd = Accept(conn_request_skt, (struct sockaddr *) &caddr, &addrlen);  
        
        /* fork a new process to serve the client on the new connection */
		if((childpid=fork())<0) 
		{ 
			/* fork() return -1 to the parent if error accours in creating the process */
			printf("Error: fork() failed");
			close(sockfd);
		}
		else if (childpid > 0)
		{ 
			/* fork() return >0 (which is childID) to the parent process */
			/* parent process */
			close(sockfd);
			/* close connected socket handling the zombie too */	
			signal(SIGCHLD,zombie_handler);
		}
		else
		{
			/* fork() return 0 to the child process */
			/* child process */
			/* here we close the passive one and we enter the server_service  */
			close(conn_request_skt);	/* close passive socket */
			/* main child service */        
			server_service(cSSL,sockfd);
			ShutdownSSL();
			close(sockfd);
			break;
		}    
    }
    exit(0);
}

/* this is the main function of the program, it execute the service required by the server 
 * service consist in sending a message to the client at the beginning and then in listaning
 * in the buffer for messages (of any kind) til the CLOSE message... 
 * params: as params we have the identifier of the socket connected
 * return: void
 * */
void server_service(SSL *certSSL, int socket){ 
	size_t len;
	
	printf("Connection from %d accepted... Sending regards\r\n",socket);
	strcpy(buf, "HELLO_CLIENT!");

	len = strlen(buf);
	if(SSL_write(certSSL, buf, len) != len)
	{
	    printf("Write error\n");
	    return;
	}

	printf("waiting for echo messages...\n");
	while((len=SSL_read(certSSL, rbuf, BUFLEN))>0)
	{
		rbuf[len]='\0';
		if(strcmp(rbuf,"CLOSE")==0)
		{
			printf("Client Requested to close the connection! Closing...\n");
			return;
		}
		printf("Received message from socket %03u : [%s]\n", socket, rbuf);
	}
	
	printf("Read error/Connection closed\n");
}


/* procedure for handling the zombie process
 * params: none
 * return: void (it doesnt expect a return variable)
 * */
void zombie_handler()
{
    //wait all child termination
    while(waitpid((pid_t)(-1),0,WNOHANG)>0){}
    printf("(!!)I'm killing the zombie!\n");
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

void InitializeSSL()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void DestroySSL()
{
    ERR_free_strings();
    EVP_cleanup();
}

void ShutdownSSL(SSL *certSSL)
{
    SSL_shutdown(certSSL);
    SSL_free(certSSL);
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
	/* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

