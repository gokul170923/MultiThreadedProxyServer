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
#include <stdlib.h>     // malloc() 
#include <string.h>     // memset()
#include <netdb.h>      // getaddinfo() 
#include <arpa/inet.h>  // for ip related tasks , read LearnInet

int main(){
        
        return 0;
}


