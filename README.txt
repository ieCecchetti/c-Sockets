-------------------------------//IEC//-------------------------------------
----------------------WELCOME IN MY SOCKET REPO----------------------------
---------------------------------------------------------------------------
HI mates,

this files in the various folders are some trials i did in my preparation 
of Distribute programming course at University!

Hope they can help someone of you

-------------------------------> INFOS ABOUT THE REPO----------------------------------
1-TCP
echo-reply)
The tcp one is a simple TCP socket that enstablish a connection between two
peers and make them communicate.
Apart from the beginning Syncronization the comunication is monodirectional. 
the Client speak to the server and the server print it in the shell.

file sender) is a simple ftp comunication between two peers
Client connets with the server asking for some files(any format is ok), server check
avalaibility and sent those back to the client
files must be in the same folder of server (i think... ?to check).

2-UDP
The UDP sample is a trial of an udp comunication. 
Easily, the server relize an echo fuction printing whatever it receive from 
the different clients.
Clients instead, it connects to the server and then start write send messages
taken in input from the keyboard.

3-SSL (template)
Performs a SSL connection between two peers... 
i never run this code.
The sketch compiles succesfully but i cannot resolve the problems with my certificate
because im not the best linux user so i found difficulties in understand the problem.
However i gather some info and all should be fine apart from that problem     

4-XDR 
Some sketches relised during labs. Exchange of message mixed with xdr comunication


---------------------------------->CREATION of EXECUTABLE & RUN------------------------
Inside every folder there is a just configured makefile, all you have to do for
run the scripts is to enter the folder like 

$ cd tcp
$ cd echo_response
$ make 
and then in two differents terminal
#terminal 1 (will be the server at port 1500)
$ Server/Server 1500
#terminal 2 (will be the client that will connect with the server at 127.0.0.1:1500)
$ Client/Client 127.0.0.1 1500


Enjoy my repo and makes me know what do you think about it! 

-------------------------------//NAN//-------------------------------------
----------------------PLEASE ENJOY AND STARR!!-----------------------------
---------------------------------------------------------------------------
