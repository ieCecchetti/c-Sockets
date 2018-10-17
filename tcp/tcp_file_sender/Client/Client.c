/* Simple Socket Client */
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>
#include    <sys/stat.h>
#include    <inttypes.h>
#include    "../errlib.h"
#include    "../sockwrap.h"
#include    <unistd.h>
#include    <time.h>

#define BUFLEN	1024 /* BUFFER LENGTH */
char     	   buf[BUFLEN];		/* transmission buffer */
char	       rbuf[BUFLEN];	/* reception buffer */

/* FUNCTION PROTOTYPES */
int client_service(int socket, char *filename);


/* GLOBAL VARIABLES */
char *prog_name;
short Max_con_temps=5;
struct stat st;


int main(int argc, char *argv[])
{
    uint16_t   tport_n, tport_h;	/* server port number (net/host ord) */
    int		   s;
    int		   result;
    struct sockaddr_in	saddr;		/* server address structure */
    struct in_addr	sIPaddr; 	/* server IP addr. structure */

    //store the name of the program, in that case ./Client0
    prog_name = argv[0];
    
    /* argc contains the main arguments so the caller parameters...
     * lets store 'em, but before, we check if the minimum of them are reached */    
    if(argc < 4)
    {
        printf("Usage: %s <server_addr> <port number> <file_name>\n", prog_name); 
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
    s = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //

    /* prepare address structure */
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port   = tport_n;
    saddr.sin_addr   = sIPaddr;

    /* connect */
    /* This function call the connect, if the server is impossible to reach then
    will be called a err_sys that exit from the program*/
	int conn_stat = connect(s, (struct sockaddr *) &saddr, sizeof(saddr)); 
    
    /* post operation-connection check */
    if(conn_stat!=0){
		close(s);
		err_sys ("Error: Impossible to reach the server.");   
	} 
    
    /* main client loop */
    /* if its possible to connect to the server then execute the Service 
     * service qill be execute untill there are arguments on the caller (#argc times)
     * if one of the "+GET file" return error "-ERR" then program exit from cycle and terminate*/
    int success=0;
    for(int i=0;i<argc-3;i++)
    {
        success=client_service(s, argv[i+3]);
        if(success==-1)
			break;
    }
    
    /* Then close the socket safetly by sending a notification (quit) to him */
    strcpy(buf,"QUIT\r\n");
    size_t	len;
    len = strlen(buf);
    writen(s, buf, len);

    close(s);
    exit(0);
}

/* this is the main function of the program, it execute the service required by the client 
 * params: as params we have the identifier of the socket connected and the filename of the file required
 * return: int 
 * 				(-1) if an error accured during the comunication between client server- transfer failure
 * 				( 0) if transfer is complete and the size of the file created match the size sended by server
 * */
int client_service(int socket, char *filename)
{                                          
    char success='t'; 
    uint32_t size=0,modification_ms=0;
    FILE *fp;
    
    /* sending the get request to the server */
    sprintf(buf, "GET %s\r\n",filename);
    size_t	msg_len;
    msg_len = strlen(buf);
    if(writen(socket,buf,msg_len) != msg_len){
        printf("Error: Fail in sending data! \n");
        return -1;
    }  
    
    //now wait till the acceptance from the server
    size_t n;        
    n=recv(socket, rbuf, 5, 0);
        if (n < 0)
            {
				/* if recv retuns <0 means that an error accours in reading(error can be studied-errno) */
                printf("Read error\n");
                return -1;
            }
            else if (n==0)
            {
				/* if recv retuns 0 means that the connection is closed */
                printf("Error: Impossible to reach Server anymore \n");

            }
            else
            {
				/* recv return lenght(>0) in byte if no err accours */
                rbuf[n]=0;
                /* if the rensponse is -ERR or an unknown one then it exits */
                if(strcmp(rbuf,"+OK\r\n")!=0)
                {
                    printf("Error: Server didnt accept the Request. <-ERR>\n"); 
                    return -1;                   
                }
            }
    
    /* wait for the receive of the size */
    if(recv(socket, &size, sizeof(uint32_t), 0)!= sizeof(uint32_t)) 
    {
        printf("Error(size): Read error\n");
        return -1;
    }
    else
        size = ntohl(size);
          
    /* wait for the receive of the last modification date */
    if(recv(socket, &modification_ms, sizeof(uint32_t), 0)!= sizeof(uint32_t)) 
    {
        printf("Error(ms_date): Read error\n");
        return -1;
    }  
    else
        modification_ms = ntohl(modification_ms);
    
    /* if all is ok till now then create a file with the same name of the requested one */
    fp = fopen(filename,"w");       
    
    /* client loop */
    /* it continue to receive file untill the size received match the one that 
     * the server sent before or untill errors accours*/
    uint32_t rec_size=0;
    while(rec_size<size){
        n=recv(socket, rbuf, BUFLEN-1, 0);
        if (n < 0)
            {
                printf("Error: Read error\n");
                success='f';
                break;
            }
            else if (n==0)
            {
                printf("Error: Server shut-down during transfering, file will be broken.\n");
                success='f';
                break;
            }
            else
            {
                fwrite(&rbuf,n,1,fp);
                rec_size+=n;
                //printf("(!!) Downloading <%s> ...Trasfered: %d/%d byte \r",filename, rec_size,size);
            }
    }
    /* close the created file */
    fclose(fp);
    
    /* if success = t then no errors accours in the process... so we need 
     * only to confirm that by checking the sizes of the 2 files */
    if(success=='t'){
		stat(filename, &st);
		if(st.st_size != size)
		{
			/* no error accours but size unmatches, possible error due to 
			 * the structure of the file */
			printf("Error: Something went the size unmatches.\n");
			return -1;
		}
		else{
			/* files sizes matches, all ok! */
			printf("(!!)Success: File: <%s> Trasferred [size: %d, last_mod: %d]\n\n",
					filename, size, modification_ms);	
			return 0;
		}
	}
	return -1;
}

    




