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
#include <sys/types.h>
#include <iostream>


// creates and binds a server socket
int main(int argc, char** argv) {
    
    int new_sock;
    struct sockaddr_in client_addr;
    int clientlen = sizeof(client_addr);
    
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
    
    if (listen(sock_fd,5) < 0) {
        perror("error on listen!/n");
        return -1;
    } else {
        std::cout << "jads;lfjkasd" << std::endl;
    }
    
    printf("HEllo WORKDLD 1 ");

    // START LISTENING LOOP FOR ACCEPT

    while (1) {
        printf("HEllo WORKDLD 2 ");

        new_sock = accept(sock_fd, (struct sockaddr *) &client_addr, (socklen_t*) &clientlen);
        printf("HEllo WORKDLD 3 ");

        if (new_sock < 0) {
            printf("error on accept!\n");
            return -1;
        }
        
        
        
    }
    
    return 0;
}

