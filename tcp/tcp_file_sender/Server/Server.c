/* Simple Socket Server */
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>
#include    <inttypes.h>
#include    <sys/stat.h>
#include    <sys/wait.h>
#include    "../errlib.h"
#include    "../sockwrap.h"

#define BUFLEN	1024 			/* BUFFER LENGTH */
#define maxConn 1; 			    /* fix #max of connections */
char     	   buf[BUFLEN];		/* transmission buffer */
char	       rbuf[BUFLEN];	/* reception buffer */

/* FUNCTION PROTOTYPES */
void server_service(int socket);
void send_error(int socket);
void zombie_handler();

/* GLOBAL VARIABLES */
char *prog_name;
FILE *fp;
struct stat st;

int main(int argc, char *argv[])
{
    int		     conn_request_skt;		/* passive socket */
    uint16_t 	 lport_n, lport_h;		/* port used by server (net/host ord.) */
    int	 	     s;			        	/* connected socket */
    int          bklog = maxConn;     	/* max # of connections  */
    socklen_t 	 addrlen;
    struct sockaddr_in 	saddr, caddr;	/* server and client addresses */ 
    int		     childpid;				/* pid of child process */

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
    s = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /* bind the socket to any local IP address */
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family      = AF_INET;
    saddr.sin_port        = lport_n;
    saddr.sin_addr.s_addr = INADDR_ANY;     //Server ip_adrr is 0.0.0.0
    
    /* bind */
    /* Specify the port and the local adress we are listening to */
    Bind(s, (struct sockaddr *) &saddr, sizeof(saddr));

    /* listen */
    /* mark the referred socket as passive, that is a socket that will 
     * be used to accept incoming connection through accept() */
    Listen(s, bklog);
    conn_request_skt = s;
    /* main server loop */
    for (;;)
    {
        /* accept next connection */
        addrlen = sizeof(struct sockaddr_in);
        s = Accept(conn_request_skt, (struct sockaddr *) &caddr, &addrlen);  
        
        
        /* fork a new process to serve the client on the new connection */
        if((childpid=fork())<0) 
        { 
			/* fork() return -1 to the parent if error accours in creating the process */
            printf("Error: fork() failed");
            close(s);
        }
        else if (childpid > 0)
        { 
            /* fork() return >0 (which is childID) to the parent process */
            /* parent process */
            close(s);
            /* close connected socket handling the zombie too */	
            signal(SIGCHLD,zombie_handler);
        }
        else
        {
			/* fork() return 0 to the child process */
            /* child process */
            /* here we close the passive one and we enter the server_service  */
            close(conn_request_skt);	/* close passive socket */
            int n=0;
            /* main child loop */        
            for (;;)
            {
                n=recv(s, rbuf, BUFLEN-1, 0);
                rbuf[n]=0;            
                if (n < 0)
                {
					/* if recv retuns <0 means that an error accours in reading(error can be studied-errno) */
                    close(s);
                    break;
                }
                else if (n==0)
                {
					/* if recv retuns 0 means that the connection is closed */
                    close(s);
                    break;
                }
                else
                {
					/* recv return lenght(>0) in byte if no err accours */
					/* if recv>0 then we need to check if is a quit request 
					 * if quit the we need to close the socked and disconnect from client
					 * if not then it will be a get or an unknown request (it will be processed
					 * in the server_service function)*/
                    if (strcmp(rbuf,"QUIT\r\n")==0)
                    {
                        rbuf[n]=0;
                        close(s);
                        break;
                    }else{ 
                        /* it must be executed one time only so after i must break the loop */
                        server_service(s);
                    }
                }
            }
            break;
        }             
    }
    exit(0);
}

/* this is the main function of the program, it execute the service required by the server 
 * params: as params we have the identifier of the socket connected
 * return: void
 * */
void server_service(int socket){
    char* op;   
    char* filename;
    size_t msg_len=0;       
    uint32_t size=0,modification_ms=0;
    char success='t';    
    
    /* decoding phase */
    /* here we alloc the strings we need and store the values received from 
     * the client(file and operation type)...  */
    op = (char *) malloc(10*sizeof(char));
    filename = (char *) malloc(40*sizeof(char));		
    sscanf(rbuf, "%s %s",op, filename);
    
    /* sscanf discard blank space and /r so we need only to check if req is "get" */
    if(strcmp(op,"GET")==0)
    {
		/* if get we try to open the relative file */
        fp = fopen(filename,"r");        
        if(fp==NULL) 
        {
			/* if file couldnt be opened then send -ERR message and exit(safetly) */
            send_error(socket);
            success='f';
            goto end;
        }

        /* get the required information then (size & date) */
        if (stat(filename, &st)) {
			/* if impossible to get inf then set to 0 */
            modification_ms=0;
            size=0;
        } else {
			/* insert information in the designed variables */
            modification_ms = st.st_mtime;
            size= st.st_size;
        }

        /* send acceptance request */
        /* compose the message to sent to the client */
        sprintf(buf,"+OK\r\n");
        msg_len = strlen(buf);
        if(writen(socket,buf,msg_len) != msg_len){
            send_error(socket);
            success='f';
            goto end;
        }
             
        /* send the size to the client */  
        size = htonl(size);
        if(writen(socket,&size,sizeof(uint32_t)) != sizeof(uint32_t)){
            send_error(socket);
            success='f';
            goto end;
        } 

		/* send last modification date */
        modification_ms = htonl(modification_ms);
        if(writen(socket,&modification_ms,sizeof(uint32_t)) != sizeof(uint32_t)){
            send_error(socket);
            success='f';
            goto end;
        } 
		
		/* file send loop */
		/* send the file BUFLEN byte at time to the client until the sended
		 * size is equal to the one obtained before from the file*/
        uint32_t sendn_size=0;        
        while(sendn_size<ntohl(size)){
			msg_len = fread(buf, 1, BUFLEN, fp);	
            if(writen(socket,buf,msg_len) != msg_len){
                success='f';
                break;
		     }
		     sendn_size+=msg_len;
        } 
        
        /* safety close part, close and free all the file and variable
         * avoid memory leakage */
        end:
         /* if succes = t no errors accoures till now so all the information
         * has been sended to the client - then print the success message 
         * 								 - error message instead */
        if(success=='t')
            printf("(*) <%s> sended correctly!.\n",filename);
        else
            printf("(!!) Error: <%s> not sent, some errors accours. \n",filename);

        if(fp!=NULL)
            fclose(fp);        
        free(op);
        free(filename);
        
    }else
    {
        send_error(socket);
        printf("(!!) Error: <%s> not sent, some errors accours. \n",filename);
    }            

}

/* procedure for sending the "err" message to the client
 * 		this is a programming decision...because this part is repeated so much time 
 * 		in the code and writing like this i'm optimizing the code structure 
 * params: as params we have the identifier of the socket connected
 * return: void (it doesnt expect a return variable)
 * */
void send_error(int socket)
{
    size_t msg_len=0; 
    strcpy(buf, "-ERR\r\n");
    msg_len = strlen(buf);
    writen(socket,buf,msg_len);
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



