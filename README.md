### SOCKET-REPO
**Some easy to understand and easy to use examples of sockets**

Hi I'm IEC ... if you want know something about me, check some details [here ](http://mycoderpage.altervista.org/On-CV/index.php)

## REPO Menu

- [Intro](Intro)
- [Some basic info]()
- [Repo Content]()
   - [TCP]()
   - [UDP]()
   - [SSL]()
   - [XDR]()
- [FUTURE IMPLEMENTATION]()
- [CREATION of EXECUTABLE & RUN]()

## Intro
HI mates,

this files in the various folders are some trials i did in my preparation 
of Distribute programming course at University

Hope they can help someone of you

## Some basic info
I think that every one who come here will know for sure what is a Socket, however to makes all of you enjoy it i will resume it shortly, so also someone who come here randomly could understand

Most interprocess communication uses the client server model. These terms refer to the two processes which will be communicating with each other. One of the two processes, the client, connects to the other process, the server, typically to make a request for information. A good analogy is a person who makes a phone call to another person.
Notice that the client needs to know of the existence of and the address of the server, but the server does not need to know the address of (or even the existence of) the client prior to the connection being established. Notice also that once a connection is established, both sides can send and receive information.

The system calls for establishing a connection are somewhat different for the client and the server, but both involve the basic construct of a socket. A socket is one end of an interprocess communication channel. The two processes each establish their own socket.

The steps involved in establishing a socket on the client side are as follows:

- Create a socket with the socket() system call;
- Connect the socket to the address of the server using the connect() system call;
- Send and receive data. There are a number of ways to do this, but the simplest is to use the read() and write() system calls;

The steps involved in establishing a socket on the server side are as follows:
- Create a socket with the socket() system call;
- Bind the socket to an address using the bind() system call. For a server socket on the Internet, an address consists of a port number on the host machine;
- Listen for connections with the listen() system call;
Accept a connection with the accept() system call. This call typically blocks until a client connects with the server.
- Send and receive data;

## Repo Content
### TCP
**echo-reply**
The tcp one is a simple TCP socket that enstablish a connection between two peers and make them communicate.
Apart from the beginning Syncronization the comunication is monodirectional.  
The Client speak to the server and the server print it in the shell.

**file sender** 
is a simple ftp comunication between two peers Client connets with the server asking for some files(any format is ok), server check avalaibility and sent those back to the client files must be in the same folder of server (i think... ?to check).

### UDP
The UDP sample is a trial of an udp comunication. 
Easily, the server realize an echo fuction printing whatever it receive from  the different clients.
Clients instead, it connects to the server and then start write send messages taken in input from the keyboard.

### SSL (template)
Performs a SSL connection between two peers... 
!!I never run this code, just compiled!!
The sketch compiles succesfully but i cannot resolve the problems with my certificate because im not the best linux user so i found difficulties in understand the problem.
However i gather some info and all should be fine apart from that problem.     

### XDR 
Some sketches relised during labs. Exchange of message mixed with xdr comunication

## FUTURE IMPLEMENTATION
In the future i would like to implement this repo with a web socket too! Makes me know if you are interested so i will move on on that project rapidly!

## CREATION of EXECUTABLE & RUN
Inside every folder there is a just configured makefile, all you have to do for
run the scripts is to enter the folder like 

```
$ cd tcp
$ cd echo_response
$ make 
```
and then in two differents terminal:

>terminal 1 (will be the server at port 1500)
```
$ Server/Server 1500
```
>terminal 2 (will be the client that will connect with the server at 127.0.0.1:1500)
```
$ Client/Client 127.0.0.1 1500
```


Enjoy my repo and makes me know what do you think about it! 

**----------------------PLEASE ENJOY AND STAR!!-----------------------------**

Also take a look of my other repos... shortly i will upload also a pratical example of socket with ESP32 
lets keep in touch and take a look at that!

This site was built using [GitHub Pages](https://pages.github.com/).
