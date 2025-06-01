#include "Headers.h"

#include<stdio.h>       // printf()
#include<stdlib.h>      // malloc()
#include<string.h>      // memset()
#include <arpa/inet.h>  // ntop() etc
#include <sys/socket.h> // socket()
#include <netdb.h>       // getaddrinfo()
#include <unistd.h>       // close()
#include <sys/time.h>   // timeval
#include <errno.h>      // timout errors

#define RETRIES 2


/*
        this function takes a hostname and path and sends a http response
        that the server will send us from the original request
*/ 

void FetchResServer(const char * host,const char * path,char ** res,int * ressize){

        /*
                a http GET request is of this format 

                GET http://idk.com HTTP/1.1
                Host :idk.com:3040
                .................
                ..............
        */

        struct addrinfo * iplist   = getIP(host) , *itr = NULL;
        int serverSocketfd = -1 , tries = 0;

        for(tries = 0 ; tries < RETRIES ; tries++ ){

                // since response is a linked list i can iterate over it
                for( itr = iplist ; itr!=NULL ; itr = itr->ai_next){  
                        
                        // create a socket to connect to the server
                        serverSocketfd = socket(itr->ai_family,itr->ai_socktype,itr->ai_protocol);
                        if (serverSocketfd < 0) {
                                perror("socket");
                                continue; // Try the next address
                        }
                        
                        // initialize and set timer to the socket
                        struct timeval timeout;
                        timeout.tv_sec = 5;
                        timeout.tv_usec = 0;
                        //  send timeout
                        if (setsockopt(serverSocketfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
                                perror("setsockopt SO_SNDTIMEO");
                                close(serverSocketfd);
                                continue;
                        }
                        //  recv timout
                        if (setsockopt(serverSocketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                                perror("setsockopt SO_RCVTIMEO");
                                close(serverSocketfd);
                                continue;
                        }

                        // connect to the server
                        if(connect(serverSocketfd,itr->ai_addr,itr->ai_addrlen) < 0){
                                perror("connect");
                                close(serverSocketfd);
                                continue;
                        }

                        // lets make a minimal GET request of the one that ckient sent
                        char request[1024];
                        snprintf(request, sizeof(request),
                        "GET /%s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "Connection: close\r\n\r\n", path, host);

                        // send http req to server
                        if (send(serverSocketfd, request, strlen(request), 0)  < 0 ) {
                                perror("send");
                                close(serverSocketfd);
                                continue;
                        }

                        /*      RECIEVE THE RESPONSE DYNAMICALLY        */
                        int responsebufferSize = 1024;
                        *res = (char*)malloc(responsebufferSize);
                        if(*res==NULL){
                                *ressize = -1;
                                perror("malloc");
                                close(serverSocketfd);
                                continue;
                        }

                        int total_bytes_recieved = 0;
                        int curr_bytes_recieved = 0;
                        // this is done to dynamically keep track of the size of the respose

                        while(( curr_bytes_recieved = recv(serverSocketfd,
                                *res +total_bytes_recieved,responsebufferSize-total_bytes_recieved,0) ) > 0){

                                        total_bytes_recieved += curr_bytes_recieved;

                                        if(total_bytes_recieved==responsebufferSize){
                                                responsebufferSize *= 2;
                                                *res = (char*)realloc(*res,responsebufferSize);

                                                if(*res == NULL) {
                                                        perror("realloc");
                                                        curr_bytes_recieved = -1;
                                                break;
                                        }
                                }

                        }

                        if (curr_bytes_recieved == -1) {
                                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                        fprintf(stderr, "recv: timeout occurred while waiting for server response\n");
                                } else {
                                        perror("recv");
                                }
                                free(*res);
                                *res = NULL;
                                *ressize = -1;
                                close(serverSocketfd);
                                continue;
                        }

                        // store the response size
                        (*res)[total_bytes_recieved] = '\0';
                        *ressize =total_bytes_recieved;

                        // close socket and break
                        close(serverSocketfd);
                        freeaddrinfo(iplist);
                        return;
                }
        }
        
        // at this point no data is recieved
        *res = NULL;
        *ressize = -1;
        fprintf(stderr, "Failed to connect to any server address\n");
        freeaddrinfo(iplist);

}