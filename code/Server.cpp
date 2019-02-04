//
//  ServerProgram.cpp
//  
//
//  Created by Ian Squiers on 1/30/19.
//

#include "Server.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

// creates and binds a server socket
int main(int argc, char** argv) {
    
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock_fd < 0) {
        printf("error opening socket\n");
        return -1;
    }
    
    int portnum = 7979;
    
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(portnum);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock_fd, (struct sockaddr*) &myaddr, sizeof(myaddr)) < 0) {
        // error binding socket
        printf("error binding socket\n");
        return -1;
    }
    
    printf("opened and bound socket!\n");
    
    listen(sock_fd)
    
    // START LISTENING LOOP FOR ACCEPT
    
    while (1) {
        new_sock = accept(sock_fd, (struct sockaddr *) &client_addr, &clientlen)
        
        if (new_sock < 0) {
            printf("error on accept!\n")
            return -1;
        }
        
        
        
    }
    
    return 0;
}

