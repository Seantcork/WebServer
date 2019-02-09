//
//  ServerProgram.cpp
//
//
//  Created by Ian Squiers on 1/30/19.
//

/*
 
 IMPLIMENTATION TODO:
     socket timeouts
     splitting up large messages
     default filepaths
     check file permissions (can ifstream but unsuccessful read?)
     how to return messages through socket
     100 Continue (extra)
 
 COMPILE ISSUES:
    with -std=c++11 flag, can use static dec of map but breaks bind comparison
    some issue printing strings with DEBUGPRINT
 
 KNOWN BUGS:
 
 THOUGHTS:
    Split into main.cpp, messagehandler.cpp, utils.cpp
 
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

//static map<string, string> ftypes = { //utils
//    { ".gif", "image/gif"  },
//    { ".jpg", "image/jpeg" },
//    { ".png", "image/png"  },
//    { ".txt", "text/plain" },
//    { ".html", "text/html" }
//};

static map<string, string> ftypes;

string filetype(string path) { //utils
    string suffix = path.substr(path.find_last_of("."));
    //DEBUG_PRINT("Filepath suffix: %s", suffix);
    cout << suffix << endl;
    string filetype = ftypes.find(suffix)->second;
    //DEBUG_PRINT("Filetype Found: %s", filetype);
    cout << filetype << endl;
    return filetype;
}

string get_date() { //utils
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[MAXREQ];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer,MAXREQ,"Date: %a, %d %b %G %T %T\n",timeinfo);
    string tstring = buffer;
    return tstring;
}

string generate_response(string http_type, string filepath) {
    string response;
    string status;
    string ctype;
    string clen;
    string date;
    
    ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (file.fail()) {
        //file does not exist 404
        return "404 Not Found";
    }
    streamsize fsize = file.tellg();
    file.seekg(0, ios::beg);
    
    DEBUG_PRINT("FILESIZE READ: %s", fsize);
    
    if (fsize > MAXURI) {
        //handle splitting of response
    }
    
    std::vector<char> fdata(fsize);
    if (file.read(fdata.data(), fsize)) //data successfully read
    {
        status = http_type +  " 200 OK";
        date = get_date();
        ctype = "Content-Type: " + filetype(filepath) + "\n";
        clen = "Content-Length: " + to_string(fsize) + "\n";
        response = status + date + ctype + clen + "\n" + fdata.data();
        return response;
    } else {
        // unable to read (permissions)
        return "403 Forbidden";
    }
    return "ERROR";
}

int handle_request(char *msg) {
    DEBUG_PRINT("handling request\n");
    
    int get = 0, http1 = 0, http11 = 0, goodreq = 1;
    string http_type;
    string reply;
    
    // Parse request
    const char *request;
    request = strtok(msg, " ");
    while(request != NULL){
        if(strcmp(request, "GET") == 0){
            get = 1;
        }
        if(strcmp(request, "HTTP/1.0") == 0){
            http1 = 1;
            http_type = "HTTP/1.0";
        }
        if(strcmp(request, "HTTP/1.1") == 0){
            http11 = 1;
            http_type = "HTTP/1.1";
        }
        request = strtok(NULL, " ");
    }
    
    // TODO: GET THE FILEPATH FROM THE REQ
    string filepath = "";
    
    if(!get || !(http1 && http11)) { // if get is bad or neither http req
        goodreq = 0;
        reply = "404 Bad Request";
    } else {
        reply = generate_response(http_type, filepath);
    }
                                         
    // tell to close the socket or not
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
        printf("error binding socket\n");
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


