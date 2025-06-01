/*
        Main new library : <sys/socket.h>     
        This header provides core socket-related functions / system calls , structures, and constants
        which are then  used in both client and server code ( it is a system level header)                 

*/

#include "Headers.h"

#include<stdio.h>       // printf()
#include<stdlib.h>      // malloc()
#include<string.h>      // memset()
#include <arpa/inet.h>  // ntop() etc
#include <sys/socket.h> // socket()
#include <netdb.h>       // getaddrinfo()
#include <unistd.h>       // close()
#include <pthread.h>    // threads
#include <sys/time.h>   // timeval


#define PORT "3490"        // Port to listen on
#define BACKLOG 1          // How many pending connections queue will hold
#define BUFFR_SIZE 1024    // size of buffer to recive data from client



void * handle_client( void  * arg){

        // retrive the essentials ( canbe skipped)
        ThreadArg *thread_arg = (ThreadArg *)arg;
        int newfd = thread_arg->client_fd;
        LRUCache *cache = thread_arg->cache;

        // Detach the thread ( itself) , to clean up automatically when it finishes
        pthread_detach(pthread_self());

        // Set timeouts for client socket
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt SO_RCVTIMEO");
            close(newfd);
            free(thread_arg);
            return NULL;
        }
        if (setsockopt(newfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt SO_SNDTIMEO");
            close(newfd);
            free(thread_arg);
            return NULL;
        }


        /*      RECIEVE THE DATA FROM CLIENT            */
        char buffer[BUFFR_SIZE];
        ssize_t bytes_received = recv(newfd,buffer,BUFFR_SIZE-1,0);
        if (bytes_received == -1) {
                perror("recv");
                close(newfd);
                free(thread_arg);
                return NULL;
        }
        buffer[bytes_received] = '\0';  // null terminator
        printf("Received from client:\n %s\n", buffer);
        // something like  GET http://idk.com:3040/ HTTP/1.1 ....


        /*         REQUEST PHASE 2 THE DATA FOR THE HTTP REQUEST        */
        char * res = NULL;
        int reslen = -1;
        FetchResCache(buffer,bytes_received,&res,&reslen,cache);
        // incase we didnt recieve desired response send a error
        if(res==NULL || reslen==-1){
                res = "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 53\r\n"
                        "\r\n"
                        "<html><body><h1>500 Internal Server Error</h1></body></html>";
                reslen = strlen(res);
        }

        /*      SEND THE DATA BACK TO CLIENT          */

        ssize_t total_bytes_sent = 0;
        while(total_bytes_sent<reslen){
                int current_sent = send(newfd,res+total_bytes_sent,reslen-total_bytes_sent,0);
                if(current_sent == -1){
                        perror("send");
                        break;
                }
                total_bytes_sent += current_sent;
        }

        printf("Response sent to client succesfully Terminate connection.\n");
        printf("\n***********************************************\n");

        // Free the response if it was dynamically allocated
        if (res != NULL && reslen != strlen("HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 53\r\n"
                        "\r\n"
                        "<html><body><h1>500 Internal Server Error</h1></body></html>")) {
                free(res);
        }

        /*              TERMINATE YOUR CONNECTION         */
        close(newfd);
        free(thread_arg);
        return NULL;
}


int main(int argc,char * argv[]){

        /*              GET MY IP            */

        struct addrinfo req, *res;
        // req will tell the library what we need and the response is lined list of type addrinfo

        memset(&req,0,sizeof(req)); // remove garbage value

        req.ai_family = AF_UNSPEC;      // ipv4 or ipv6
        req.ai_socktype = SOCK_STREAM;  // TCP
        req.ai_flags = AI_PASSIVE;      // if hostname is null return my ip

        int status = getaddrinfo(NULL,PORT,&req,&res);
        // hostname , port , req struct addr , res pointer addr 

        if(status!=0){
                fprintf(stderr,"getaddrinfo gave an error: %s\n",gai_strerror(status));
                // gai_strerro converts the error given by getaddrinfo to a readable string
                exit(1);
        }

        /*              SOCKET STARTS HERE      */

        int socketfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
        // (ip type , sockettype , protocol) retruns a file descripter for our socket
        
        if(socketfd == -1){
                perror("socket");
                exit(1);
        }

        /*
                Sometimes, you might notice, you try to rerun a server and bind() fails, 
                claiming “Address already in use.” What does that mean? 
                Well, a little bit of a socket that was connected 
                is still hanging around in the kernel, and it’s hogging the port. 
                You can either wait for it to clear (a minute or so), 
                or add code to your program allowing it to reuse the port, 
                like this:
        */
        int yes=1;
        // lose the pesky "Address already in use" error message
        if (setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1){
            perror("setsockopt");
            exit(1);
        } 

        
        /*              BIND SOCKET TO A IP:PORT      */

        int bindstatus = bind(socketfd,res->ai_addr,res->ai_addrlen);
        //binding our file disc to socketaddr(ip:port)
        if(bindstatus==-1){
                perror("bind");
                close(socketfd);
                exit(1);
        }

        freeaddrinfo(res);      // free space not needed


        /*             START LISTENING               */
        int listenstatus = listen(socketfd,BACKLOG);
        // backlog is kind off the capacity of accept queue
        if(listenstatus==-1){
                perror("listen");
                exit(1);
        }
        printf("Server is listening on port %s...\n", PORT);

        // initialize our lru to be used
        int capacity = 5;
        LRUCache * cache = createLRU(capacity);
        if(cache == NULL) {
                fprintf(stderr, "Failed to initialize LRU cache\n");
                exit(1);
        }

        
        /*             SERVE THE CLIENT / ACCEPT ( single in this case)        */

        while(1){

                struct sockaddr_storage client_addr;    // kernel fill this with accepted clien's address
                int addr_size = sizeof(client_addr);
                /*
                LORE of SOCKADDR_STORAGE LOL
                the programmers thaught altough we only have ipv4 right now 
                we might get more address types in future then our bind() , accept() 
                and other function wont work hence lets make a inteface 
                ( just a skeleton that every upcoming address must follow ) 
                so they can be casted in this inteface , this was cool 
                but they didnt knew what sizes would the future address have 
                hence it was not large wnough when ipv6 came hence now we need sockaddr_storage
                */
                int newfd = accept(socketfd,(struct sockaddr*)&client_addr,&addr_size);
                // need pointer to addr_size because accept modifies it accordingly also accept block until clinet is available
                if(newfd==-1){
                        // coudnt accept
                        perror("accept");
                        exit(1);
                }


                // Create thread argument ( this is passed to a function that a thread performs)
                ThreadArg *thread_arg = (ThreadArg *)malloc(sizeof(ThreadArg));
                if(!thread_arg){
                    perror("malloc thread_arg");
                    close(newfd);
                    continue;
                }
                thread_arg->client_fd = newfd;
                thread_arg->cache = cache;

                // Spawn a worker thread to handle the client and pass the arguments
                pthread_t thread;
                if(pthread_create(&thread, NULL, handle_client, thread_arg) != 0){
                        /* int pthread_create(pthread_t *thread,
                                const pthread_attr_t *attr,  // ← this is the NULL part
                                void *(*start_routine)(void *),
                                void *arg
                        ); 
                        thread always takes a void * function( void * )
                        */
                    perror("pthread_create");
                    free(thread_arg);
                    close(newfd);
                }

        }
        close(socketfd);
        freeLRU(cache);
        
        return 0;
}