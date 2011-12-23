/*======================================================== 
** University of Illinois/NCSA 
** Open Source License 
**
** Copyright (C) 2011,The Board of Trustees of the University of 
** Illinois. All rights reserved. 
**
** Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
** Copyright (C) Sam King
**
** Permission is hereby granted, free of charge, to any person obtaining a 
** copy of this software and associated documentation files (the 
** Software), to deal with the Software without restriction, including 
** without limitation the rights to use, copy, modify, merge, publish, 
** distribute, sublicense, and/or sell copies of the Software, and to 
** permit persons to whom the Software is furnished to do so, subject to 
** the following conditions: 
**
** Redistributions of source code must retain the above copyright notice, 
** this list of conditions and the following disclaimers. 
**
** Redistributions in binary form must reproduce the above copyright 
** notice, this list of conditions and the following disclaimers in the 
** documentation and/or other materials provided with the distribution. 
** Neither the names of Sam King or the University of Illinois, 
** nor the names of its contributors may be used to endorse or promote 
** products derived from this Software without specific prior written 
** permission. 
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
** IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
** ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
** SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/

#include "MyServerSocket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MyServerSocket::MyServerSocket(int port)
{
    struct sockaddr_in server;
    int one = 1;
  
    // set up the server socket
    serverFd = socket(AF_INET,SOCK_STREAM,0);
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons((short) port);
    
    if (setsockopt(serverFd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(int)) == -1) {
        throw MySocketException("error with set socket opts");
    }
    
    if( bind(serverFd,(struct sockaddr *) &server, sizeof(server)) ==-1){
        char str[1024];
        sprintf(str,"could not bind to port %d",port);
        throw MySocketException(str);
    }	
    
    //set up a listen queue
    listen(serverFd, 10);
}

MySocket *MyServerSocket::accept()
{
    //check that the sockFd is valid
    
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int clientFd = ::accept(serverFd, (struct sockaddr *) &client, &len);
    
    if(clientFd<0) {
        throw MySocketException("Accept function call failed");
    }
    
    return new MySocket(clientFd);
}
