/*
        In my words : 
        Rember studing the flow of how hostname is resolved to IP address
        our computer has a local dns ip stored ,
        so the flow is our computer -> local dns -> rootdns -> top level dns - > authoritative dns
        this library just proves a interface such that I input a hostname and it calls my local dns 
        ( which kickstarts the whole process) and returns me the resolved IP address
        , just like what a browser does for me . 

        How to use : 
        gethostbyname() and getaddrinfo() are used to resolve hostnames to IP addresses

        differnce : 
        
        gethostbyname(): This is an older function , only supports IPv4 addresses. returns a struct hostent, 
        which contains the host's information , 
        Not thread-safe. If multiple threads call gethostbyname() simultaneously, 
        it can cause problems because the internal data structures used bu it may be shared between threads.

        
        getaddrinfo() : This is a more modern, flexible function to handle both IPv4 and IPv6 addresses.
        It can return multiple addresses (e.g., both IPv4 and IPv6 addresses for the same hostname).
        It returns a linked list of struct addrinfo, which provides detailed address information.
        Thread-safe. This function is designed to be safe to use in multi-threaded programs.

        read more this is just the tip We will be using only getaddrinfo()
*/

#include <stdio.h>      // printf() 
#include <stdlib.h>     // malloc() , exit(1)
#include <string.h>     // memset()
#include <netdb.h>      // getaddinfo() 
#include <arpa/inet.h>  // for ip related tasks , convert the ip to different format

int main(int argc , char * argv[]){

        if(argc==1){
                fprintf(stderr,"hostname not entered\n");
                return 1;
        }
        
        struct addrinfo req , *res;    
        // req will tell the library what we need and the response is lined list of type addrinfo

        int status;     // getinfoaddr() also returns a status

        memset(&req,0,sizeof(req));     // remove garbage from memory

        req.ai_family = AF_INET;
        req.ai_socktype = SOCK_STREAM;
        /*
                the addrinfo struct has these members ( more)
                struct addrinfo {
                    int ai_flags;
                    int ai_family;      // address info family (like AF_INET for ipv4 or AF_INET6 for ipv6)
                    int ai_socktype;    // socket type (like SOCK_STREAM for tcp or SOCK_DGRAM for udp)
                    int ai_protocol;    // protocol (like IPPROTO_TCP)
                    struct sockaddr *ai_addr;      // Pointer to struct sockaddr (the actual address)
                    char            *ai_canonname; // Canonical hostname (optional)
                    struct addrinfo *ai_next;      // Pointer to next result (linked list)
                };

        */

        status = getaddrinfo(argv[1],NULL,&req,&res);
        // NULL param : you can request port as well , on success we get 0 else non zero

        if(status!=0){
                fprintf(stderr,"getaddrinfo gave an error: %s\n",gai_strerror(status));
                // gai_strerro converts the error given by getaddrinfo to a readable string
                return 1;
        }

        for(struct addrinfo * itr = res;itr!=NULL;itr = itr->ai_next){
                // since response is a linked list i can iterate over it

                char ipAsString[INET_ADDRSTRLEN];
                // this is just to store ip as a string
                // inet_addrstrlen is a Macro in inet which tells ipv4 address length as a string

                struct sockaddr_in *sockaddr_ptr = (struct sockaddr_in *) itr->ai_addr;
                // in structure od addrinfo ai_addr was a adress of type struct sockaddr 
                // tyoe sockaddrrepresents all kinds of sicket addresss in networking 
                // we just type casted it to type sockaddr_in for ipv4 adress only

                inet_ntop(AF_INET,&sockaddr_ptr->sin_addr,ipAsString,sizeof(ipAsString));
                // convert the ip (currently as bit) to string 
                //params : AF_INET(type ipv4) , source , destination , size of destinatin

                printf("Resolved IP Address: %s , Canonical name : %s\n",ipAsString,itr->ai_canonname);

        }

        return 0;
}


