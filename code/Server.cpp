//
//  ServerProgram.cpp
//
//
//  Created by Ian Squiers & Sean Cork on 1/30/19.
//

/*
 
 IMPLIMENTATION TODO:
    socket timeouts
    default filepaths
    http 1.1 functionality (take multiple requests)
    check thread exiting

     splitting up large messages
     	-meaning recving get messages that are pretty long. Need to think about how to do that.
     	-Idea would be to get to where the message header tells you how long the messaage is. and 
     	-call recv till you have that amount of messages.


     Seting mutex when sending and writing data in HTTP/1.1

    no guarentee that we will send everything with the just one send especially in HTtp1.1.
    need to determine how to do that.
    Test to see if 403 works. Will edit file permisions and test it out.
     100 Continue (extra)
 
 COMPILE ISSUES:
    some issue printing strings with DEBUGPRINT
 
 KNOWN BUGS:
    spaces in filenames
 
 THOUGHTS:
    Split into main.cpp, messagehandler.cpp, utils.cpp
    CLRF vs LF
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
#include <utility>

#define DEBUG_ME 1
#define DEBUG_PRINT(format, ...) if(DEBUG_ME) {\
printf("%s:%d -> " format "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__);\
fflush(stdout);}

const int MAXURI = 255;
const int MAXREQ = 80; // good RoT for this?

using namespace std;

static map<string, string> ftypes = { //utils
    { ".gif", "image/gif"  },
    { ".pdf", "image/pdf" },
    { ".png", "image/png"  },
    { ".txt", "text/plain" },
    { ".html", "text/html" }
};

struct arg_struct {
    string arg1;
    int arg2;
}args;

string filetype(string path) { //utils
    string suffix = path.substr(path.find_last_of("."));
    cout << suffix << endl;
    map<string, string>::iterator find;
    find = ftypes.find(suffix);
    string filetype;
    if(find == ftypes.end()){
    	filetype = "cant handle request";
    }
    else{
    	filetype = find->second;

    }


    cout << filetype << endl;
    return filetype;
}

string get_date() { //utils
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[MAXREQ];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer,MAXREQ,"Date: %a, %d %b %G %T %Z\r\n",timeinfo);
    string tstring = buffer;
    return tstring;
}

char *generate_response(string http_type, string filepath, string rootdir) {
    DEBUG_PRINT("GENERATING RESPONSE");
    string response;
    string status;
    string ctype;
    string clen;
    string date;
    string type_of_file = filetype(filepath);
    

    if(type_of_file.compare("cant handle request") == 0){
        	return (char*)"404 Bad Request\n";
    }

    ifstream file;
    file.open(filepath, std::ios::binary | std::ios::ate);
    if (file.fail()) {
        filepath = rootdir + filepath;
        cout << "with rootdir : " << filepath << endl;
        DEBUG_PRINT("Appending RootDir");
        file.open(filepath, std::ios::binary | std::ios::ate);
        if (file.fail()) {
            //file does not exist 404
            DEBUG_PRINT("HERE");
            return (char*)"404 Not Found\n";
        }
    }
   
    streamsize fsize = file.tellg();
    file.seekg(0, ios::beg);
    
    cout << "FILESIZE: " << fsize<< endl;
    
    if (fsize > MAXURI) {
        //handle splitting of response
    }
    
    std::vector<char> fdata(fsize);

    //checks to see if user can access files
//    if(access(filepath, R_OK) < 0) {
//        return (char*)"403 Forbidden";
//    }
    
    if (file.read(fdata.data(), fsize)) //data successfully read
    {
        status = http_type + " 200 OK\r\n";
        date = get_date();
        
        ctype = "Content-Type: " + type_of_file + "\r\n";
        cerr << type_of_file << endl;
       	
       
        clen = "Content-Length: " + to_string(fsize) + "\r\n";
        response = status + date + ctype + clen + "\r\n" + fdata.data() + "\r\n";
        int n = response.length();
        char *char_array = new char[n+1];
        strcpy(char_array, response.c_str());
        file.close()
        return char_array;
    } else {
        // unable to read (permissions)
        file.close()
        return (char*)"403 Forbidden";
    }
    file.close()
    return (char*)"ERROR";

}

int handle_request(char *msg, int socket, string rootdir) {
    DEBUG_PRINT("handling request\n");
    
    int get = 0, http1 = 0, http11 = 0, goodreq = 1;
    string http_type;
    string filepath = "";
    const char *reply;
    
    // Parse request
    const char *request;
    request = strtok(msg, " ");
    int pos = 0; // order of req words
    while(request != NULL){
    	cout << request << endl;
        if(!strcmp("GET", request) && pos == 0){
        	DEBUG_PRINT("IN GET")
            get = 1;
        }
        else if(!strncmp("HTTP/1.0", request, strlen("HTTP/1.0")) && pos == 2){
        	DEBUG_PRINT("IN HTTP/1.0")
            http1 = 1;
            http_type = "HTTP/1.0";
        }
        else if(!strncmp("HTTP/1.1", request, strlen("HTTP/1.1")) && pos == 2){
            http11 = 1;
            http_type = "HTTP/1.1";
        }
        else if(pos == 1) {
            filepath = request;
            printf("FILEPATH SET TO: ");
            cout << filepath << endl;
        }
        request = strtok(NULL, " ");
        pos++;
    }
    
    DEBUG_PRINT("get: %d, h0: %d, h1 %d", get, http1, http11);
    
    if(!get || !(http1 || http11)) { // if get is bad or neither http req
        goodreq = 0;
        reply = (char*)"404 Bad Request\n";
    } else {
        reply = generate_response(http_type, filepath, rootdir);
    }


    int file_sent;
    file_sent = send(socket, reply, strlen(reply) ,0);
    size_t length_sent = strlen(reply);
	if(file_sent == -1){
		cerr << "Errror sending file" << endl;
		return -1;
	}
	length_sent = length_sent - file_sent;
	while (file_sent < length_sent){
		file_sent = send(socket, reply, strlen(reply), 0);

	}
	cerr << "ERROR sending socket" << endl;

    
    DEBUG_PRINT("wrote reply");
                                         
    // tell to close the socket or not
    if (http11 && goodreq) {
        return 1;
    } else {
        return 0;
    }
}

void *new_connection(void *info) {
    
    struct arg_struct *args = (struct arg_struct *)info;
    string rootdir = args->arg1;
    int sock = args->arg2;
    
    int connection = 1;
    
	while(connection){
	    char req[MAXREQ] = {0};
	    int n = read(sock, req, MAXURI);
	    if (n < 0) {
	        cout << "error on read!/n" << endl;
	    }
	    DEBUG_PRINT("MESSAGE RECIEVED: %s\n", req);
	    if (!handle_request(req, sock, rootdir)) { // if 0 (http1.0) close the socket
	        connection = 0;
	    }
	}
	DEBUG_PRINT("Closing socket\n");
	close(sock);

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

    // if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, ))
    
    if (sock_fd < 0) {
        printf("error opening socket\n");
        return -1;
    }
    
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(portnum);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (::bind(sock_fd, (struct sockaddr*) &myaddr, sizeof(myaddr)) < 0) {
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
        struct arg_struct args;
        args.arg1 = rootdir;
        args.arg2 = new_sock;
        
        if( pthread_create( &new_thread, NULL, new_connection, (void*)&args ) < 0){
        	cerr << "Thread Creation Failed" << endl;
        	return -1;
    	}

    }
    
    return 0;
}


