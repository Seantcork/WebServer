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
#include <pthread.h>
#include <string.h>
#include <chrono>



using namespace std;

void handle_request(char *msg){
  cout << "in here" << endl;
  int get = 0;
  int http1 = 0;
  int http11 = 0;
  const char *request;
  char response[40];
  request = strtok(msg, " ");
  while(request != NULL){
    if(strcmp(request, "GET") == 0){
      get = 1;
    }
    if(strcmp(request, "HTTP/1.0") == 0){
      http1 = 1;
    }
    if(strcmp(request, "HTTP1.1") == 0){
      http11 = 1;
    }
    request = strtok(NULL, " ");
  }
  if(get == 0){
    cout << "in here" << endl;
    strcat(response, "404 Bad Request");
    cout << "after here" << endl;
  }
  cout << "in here3" << endl;
  cout << response << endl;
}

void *new_connection(void *new_sock) {
    
    int sock = (uintptr_t)new_sock;
    
    char message[256] = {0};
    //char newMessage;
    int n = read(sock, message, 255);
    if (n < 0) {
        cout << "error on read!/n" << endl;
    }
    cout << "MESSAGE RECIEVED:" << message << endl;
    handle_request(message);
    cout << "after request" << endl;
    
    close(sock);
}


// creates and binds a server socket
int main(int argc, char** argv) {
    
    int c;
    int portnum, pflag, rflag = 0;
    char *rootdir;
    
    while ((c = getopt (argc, argv, "p:r:")) != -1)
        switch (c)
    {
        case 'p':
            pflag = 1;
            portnum = atoi(optarg);
            break;
        case 'r':
            rflag = 1;
            rootdir = optarg;
            break;
        case '?':
            // err = 1;
            break;
    }
    
    // printf ("portnum = %d, rootdir = %s\n",portnum, rootdir);
    
    
    int new_sock;
    struct sockaddr_in client_addr;
    int clientlen = sizeof(client_addr);
    
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock_fd < 0) {
        printf("error opening socket\n");
        return -1;
    }
    
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
    
    cout << "HEllo WORKDLD 1 " << endl;
    
    // START LISTENING LOOP FOR ACCEPT
    
    while (1) {
        cout << "HEllo WORKDLD 2 " << endl;
        new_sock = accept(sock_fd, (struct sockaddr *) &client_addr, (socklen_t*) &clientlen);
        cout << "HEllo WORKDLD 3 " << endl;
        if (new_sock < 0) {
            cout << "error on accept!\n" << endl;
            return -1;
        }
        
        pthread_t new_thread;
        pthread_create( &new_thread, NULL, new_connection, (void*)new_sock );

    }
    
    return 0;
}


