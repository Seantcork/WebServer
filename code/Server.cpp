//
//  ServerProgram.cpp
//
//
//  Created by Ian Squiers on 1/30/19.
//

/*
 TODO:
 socket timeouts
 splitting up large messages
 default filepaths
 
 */

#include "Server.hpp"
#include <time.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <chrono>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <map>

#define DEBUG_ME 1
#define DEBUG_PRINT(format, ...) if(DEBUG_ME) {\
printf("%s:%d -> " format "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__);\
fflush(stdout);}

const int MAXURI = 255;
const int MAXREQ = 80; // good RoT for this?

using namespace std;

static map<string, string> ftypes = { //utils
    { ".gif", "image/gif"  },
    { ".jpg", "image/jpeg" },
    { ".png", "image/png"  },
    { ".txt", "text/plain" },
    { ".html", "text/html" }
};

string filetype(string path) { //utils
    string suffix = path.substr(path.find_last_of("."));
    DEBUG_PRINT("Filepath suffix: %s", suffix);
    string filetype = ftypes.find(suffix)->second;
    DEBUG_PRINT("Filetype Found: %s", filetype);
    return filetype;
}

string get_date() { //utils
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer,80,"Date: %a, %d %b %G %T %T\n",timeinfo);
    string tstring = buffer;
    return tstring;
}

string generate_response(string http_type /* <-- format as "HTTP/1.x" */, string filepath) {
    
    string status = http_type;
    string ctype = "Content-Type: " + filetype(filepath) + "\n";
    string clen = "Content-Length: ";
    
    ifstream file(filepath, std::ios::binary | std::ios::ate);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    
    std::vector<char> fdata(size);
    if (file.read(fdata.data(), size))
    {
        string cat = string(fdata.data(), size);
        string date = get_date();
        string response = status + date + ctype + clen + "\n" + fdata.data();
        cout << response;
    }
}

int handle_request(char *msg) {
    DEBUG_PRINT("handling request\n");
    int get = 0, http1 = 0, http11 = 0, goodreq = 1;
    const char *request;
    char response[40];
    
    // Parse request
    request = strtok(msg, " ");
    while(request != NULL){
        if(strcmp(request, "GET") == 0){
            get = 1;
        }
        if(strcmp(request, "HTTP/1.0") == 0){
            http1 = 1;
        }
        if(strcmp(request, "HTTP/1.1") == 0){
            http11 = 1;
        }
        request = strtok(NULL, " ");
    }
    
    if(!get || !(http1 && http11)) { // if get is bad or neither http req
        goodreq = 0;
        DEBUG_PRINT("404 error");
        // WHAT DO WE DO HERE
    }
    
    
    if (http11 && goodreq) {
        return 1;
    } else {
        return 0;
    }
}

void *new_connection(void *new_sock) {

    int sock = (uintptr_t)new_sock;
    
    char req[MAXREQ] = {0};
    int n = read(sock, req, MAXURI);
    if (n < 0) {
        cout << "error on read!/n" << endl;
    }
    DEBUG_PRINT("MESSAGE RECIEVED: %s\n", req);
    if (!handle_request(req)) { // if 0 (http1.0) close the socket
        DEBUG_PRINT("Closing socket\n");
        close(sock);
    }
}


// creates and binds a server socket
int main(int argc, char** argv) {
    
    int c, err, portnum, pflag, rflag = 0;
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
            err = 1;
            break;
    }
    if (err) {
        perror("error on commandline");
    }
    
    DEBUG_PRINT("portnum %d, rootdir %s", portnum, rootdir);
    
    int sock_fd, new_sock, clientlen;
    
    struct sockaddr_in client_addr;
    clientlen = sizeof(client_addr);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sock_fd < 0) {
        printf("error opening socket\n");
        return -1;
    }
    
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(portnum);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock_fd, (struct sockaddr*) &myaddr, sizeof(myaddr)) < 0) {
        perror("error binding socket\n");
        return -1;
    }
    DEBUG_PRINT("opened and bound socket!\n");
    
    if (listen(sock_fd,5) < 0) {
        perror("error on listen!\n");
        return -1;
    }

    while (1) {
        new_sock = accept(sock_fd, (struct sockaddr *) &client_addr, (socklen_t*) &clientlen);
        DEBUG_PRINT("Connection found and accepted\n")
        if (new_sock < 0) {
            cout << "error on accept!\n" << endl;
            return -1;
        }
        
        pthread_t new_thread;
        pthread_create( &new_thread, NULL, new_connection, (void*)(size_t)new_sock );
    }
    
    return 0;
}


