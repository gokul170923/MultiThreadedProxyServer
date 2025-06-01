/*
        In my words : 
        Remember studing the flow of how hostname is resolved to IP address
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


        BYTE ORDER :
                all computers store data differently some BIG ENDIAN some LITTLE ENDIAN
                so the newtork transfer need to be standardised that standard is 
                NETWORK BYTE ORDER (BIG ENDIAN) , so to convert the ip among these three representation
                (network byte order , host byte order , presentation/printable )
                we use the <arpa/inet.h> it has functions like
                ntop() , pton()
                ntohs() , htons() , ntohl() , htonl()
                here "s" is for 16 bit data (ip etc) "l" is for 32 bit data (ip address etc)

        
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
                lets make this socket struct hierachly clear 
                [ struct addrinfo ] ──>  
                has a member struct sockaddr  * ai_addr ──>
                which can be casted to struct sockaddr_in *  for ipv4 type compatibulity ──>
                noe this has a member  struct in_addr * sin_addr (binary IP)

                struct addrinfo {
                        int              ai_flags;
                        int              ai_family;    // AF_INET or AF_INET6
                        int              ai_socktype;  // SOCK_STREAM (TCP) or SOCK_DGRAM (UDP)
                        int              ai_protocol;
                        size_t           ai_addrlen;

                        struct sockaddr *ai_addr;      // 👈 Actual address here (generic type)

                        char            *ai_canonname;
                        struct addrinfo *ai_next;      // Linked list
                };      


                struct sockaddr {
                    unsigned short sa_family;     // Address family (AF_INET, AF_INET6)
                    char           sa_data[14];   // Actual data (protocol-specific)
                };
                casted to ->
                struct sockaddr_in {
                    short            sin_family;   // AF_INET
                    unsigned short   sin_port;     // Port (in network byte order)
                    struct in_addr   sin_addr;     // 👈 Actual IP address
                    char             sin_zero[8];  // Padding
                };


                struct in_addr {
                    uint32_t s_addr;  // 👈 IP address in network byte order
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


