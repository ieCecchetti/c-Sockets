/* ssl Clien realized with the openSSL lib	
 * Main Function: Client create and setup the socketSSL
 * then it connect to the SSLServer waiting to the accept and the init message.
 * Received that it start ask the user to insert a line to send to the server
 * till user type 'close' */

#include     <stdlib.h>
#include     <string.h>
#include     <inttypes.h>
#include     "../errlib.h"
#include     "../sockwrap.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFLEN	128 	/* BUFFER LENGTH */
char    buf[BUFLEN];	/* transmission buffer */
char	rbuf[BUFLEN];	/* reception buffer */

/* FUNCTION PROTOTYPES */
int mygetline(char * line, size_t maxline, char *prompt);
void client_service(SSL *certSSL, int socket);

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile);
void InitializeSSL();
void DestroySSL();
void ShutdownSSL();

/* GLOBAL VARIABLES */
char *prog_name;


int main(int argc, char *argv[])
{
    uint16_t	   tport_n, tport_h;	/* server port number (net/host ord) */

    int sockfd, newsockfd;
    int		   result;
    struct sockaddr_in	saddr;		/* server address structure */
    struct in_addr	sIPaddr; 	/* server IP addr. structure */

	SSL_CTX *sslctx;
	SSL *cSSL;
	
	InitializeSSL();

    prog_name = argv[0];

   //store the name of the program, in that case ./Client0
    prog_name = argv[0];
    
    /* argc contains the main arguments so the caller parameters...
     * lets store 'em, but before, we check if the minimum of them are reached */    
    if(argc < 3)
    {
        printf("Usage: %s <server_addr> <port number>\n", prog_name); 
        exit(1);
    } 
    
    /* inet_aton convert into bynary the address 
     * and check if is valid, ret 1 if yes 0 is not accepted */
     /* get ip_addr */
    result = inet_aton(argv[1], &sIPaddr);                                              
    if (!result)
	   err_quit("Invalid address");

	/* get port */
    if (sscanf(argv[2], "%" SCNu16, &tport_h)!=1)
	   err_quit("Invalid port number");
    tport_n = htons(tport_h);       //htons convert to byte the port

    /* create the socket */
    printf("Creating socket\n");
    sockfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("done. Socket fd number: %d\n",sockfd);

    /* prepare address structure */
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port   = tport_n;
    saddr.sin_addr   = sIPaddr;

    /* connect */
    showAddr("Connecting to target address", &saddr);
    Connect(sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
    printf("done.\n");
    
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

    /* main client loop */
	client_service(cSSL, sockfd);
	ShutdownSSL();
	close(sockfd);
	
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


void client_service(SSL *certSSL, int socket)
{
	int n;
	printf("waiting for the message from the server...\n\r");
	
	n=SSL_read(certSSL, rbuf, BUFLEN-1);
	if (n < 0)
	{
		printf("Error: Read error\n");
		return;
	}
	else if (n==0)
	{
		printf("Error: Server shut-down during transfering, data will be broken.\n");
		return;
	}
	else
	{
		printf("Received message from socket %03u : [%s]\n", socket, rbuf);
	}
	
	while(!(strcmp(buf,"CLOSE")==0))
    {
        size_t	len;

        mygetline(buf, BUFLEN, "Enter line (max 127 char) or 'CLOSE' to end: ");
		len = strlen(buf);
		if(SSL_write(certSSL, buf, len) != len)
		{
			printf("Write error\n");
			break;
		}	
		printf("===========================================================\n");
	
	}
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
