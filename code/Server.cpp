// 
//  ServerProgram.cpp
//
//
//  Created by Ian Squiers & Sean Cork on 1/30/19.
//

/*
 
 IMPLIMENTATION TODO:
    http 1.1 functionality (take multiple requests)
    check thread exiting

     Seting mutex when sending and writing data in HTTP/1.1

    no guarentee that we will send everything with the just one send especially in HTtp1.1.
    need to determine how to do that.
     100 Continue (extra)
     make sure we recieve the whole message
     
 COMPILE ISSUES:
    some issue printing strings with DEBUGPRINT
 
 KNOWN BUGS:
    spaces in filenames
 
 THOUGHTS:
    Split into main.cpp, messagehandler.cpp, utils.cpp
    CLRF vs LF
 */

//#include "Server.hpp"
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
#include <mutex>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>


#define DEBUG_ME 1
#define DEBUG_PRINT(format, ...) if(DEBUG_ME) {\
printf("%s:%d -> " format "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__);\
fflush(stdout);}

const int MAXURI = 4000;
const int MAXREQ = 4000; // good RoT for this?

using namespace std;

static map<string, string> ftypes = { //utils
    { ".gif", "image/gif"  },
    { ".jpg", "image/jpg"  },
    { ".txt", "text/plain" },
    { ".html", "text/html" }
};

struct arg_struct {
    char* arg1;
    int arg2;
}args;

mutex mtx;
int connections = 0;

/*

Use: This function returns the filetype
of the requested source as a string.
Parameters: a string which is the specified file path
Return value: The mapped string which describes the type
of file being requested.

*/
string filetype(string path) { //utils
    string suffix = path.substr(path.find_last_of("."));
    cout << suffix << endl;
    map<string, string>::iterator find;
    find = ftypes.find(suffix);
    string filetype;

    //If the file type is not supported by our program raise a flag
    if (find == ftypes.end()){
    	filetype = "cant handle request";
    }
    else {
    	filetype = find->second;
    }
    return filetype;
}


/*

Use: Returns a string with the current date and time
Parameters: none
Return value: formatted string with time and date

*/
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

/*

Use: This Function takes the filepath and the HTTP type and 
creates and returns an appropirate response to the HTTP request.
Parameters: http_type which is either http/1.0 or http/1.1 and filepath
which is the filepath from the directory
Return value: A char* buffer which represents the HTTP response that the server is going to send.

*/
char *generate_response(string http_type, string filepath, string rootdir) {
    DEBUG_PRINT("GENERATING RESPONSE");
    string response;
    string status;
    string ctype;
    string clen;
    string date;
    string type_of_file = filetype(filepath);
    

    if (type_of_file.compare("cant handle request") == 0){
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

    
    if (file.read(fdata.data(), fdata.size())) { //data successfully read
    	
        status = http_type + " 200 OK\r\n";
        date = get_date();
        
        ctype = "Content-Type: " + type_of_file + "\r\n";
        cerr << type_of_file << endl;
       
        clen = "Content-Length: " + to_string(fsize) + "\r\n";
        response = status + date + ctype + clen + "\r\n" + fdata.data() + "\r\n";
        
        int n = response.length();
        char *char_array = new char[n+1];
        strcpy(char_array, response.c_str());
        file.close();
        return char_array;
    } else {
        // unable to read (permissions)
        file.close();
        return (char*)"403 Forbidden";
    }
    file.close();
    return (char*)"ERROR";
}

int get_file_size(std::string filename) // path to file
{
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}

/*

Use: This function takes the message recieved from the socket and the socket number. First it determines
if the GET request is correctly formated and that the HTTP request is a GET request. It then parses the filepath of
the HTTP request. If the HTTP request is formatted correctly the function calls Generate Response which generates an
HTTP message to send. The function then attempts to send the message to the client. If request is HTTP/1.0 it closes
the socket. Otherwise the socket is kept open.
Parameters: Char* msg is the buffer recieved from the socket. Int socket is the identifier for the socket number of the
request.
Return value: 1 if http1.1 and good request, otherwise return 0.

*/

int handle_request(char *msg, int socket, string rootdir) {
    DEBUG_PRINT("handling request\n");
    
    int get = 0, http1 = 0, http11 = 0, goodreq = 1, goodfile = 0;
    string http_type;
    string filepath = "";
    ifstream reqfile;
    streamsize fsize;
    const char *header;
    
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
        header = (char*)"400 Bad Request\r\n";
    } 

    else {
        if(filepath.length() == 1 && filepath.compare("/") == 0){
            filepath = "/index.html";
            filepath = rootdir + filepath;
        }
        else{
            filepath = rootdir + filepath;
            
        }
        reqfile.open(filepath, ios::binary);
        if (errno == ENOENT || reqfile.fail()) { // file does not exist
            //filepath = rootdir + filepath;
            DEBUG_PRINT("Failed: appending rootDir");
            reqfile.open(filepath, ios::binary);
            if (errno == ENOENT|| reqfile.fail()) { // file does not exist
                DEBUG_PRINT("Failed to find file");
                header = (char*)"404 Not Found\r\n";  
            }
        //does this jump clear the errno
        } else if (errno == EACCES) { // permission denied
            DEBUG_PRINT("Failed read access");
            header = (char*)"403 Forbidden\r\n";
        }

        else if (!filetype(filepath).compare("cant handle request")) {
            DEBUG_PRINT("Incompatable FileExtension");
            header = (char*)"404 Bad Request\r\n";
        }

        else {
            DEBUG_PRINT("HERE1");

            goodfile = 1;
            string type_of_file = filetype(filepath);
            string status = http_type + " 200 OK\r\n";
            string date = get_date();
            
            string ctype = "Content-Type: " + type_of_file + "\r\n";

            fsize = get_file_size(filepath);

            string clen = "Content-Length: " + to_string(fsize) + "\r\n";
            string response = status + date + ctype + clen + "\r\n";
            int n = response.length();
            char *char_array = new char[n+1];
            strcpy(char_array, response.c_str());
            header = char_array;

        }

    }
    reqfile.close();

    size_t bytes_sent;

    //send header info
    size_t bytes_left = strlen(header);
    bytes_sent = send(socket, header, strlen(header) ,0);
	if(bytes_left < 0){
		cerr << "Errror sending file" << endl;
		return -1;
	}

	bytes_left -= bytes_sent;

	while (bytes_left > 0){
        cout << bytes_left << endl;
		bytes_sent = send(socket, header, bytes_left, 0);
		bytes_left -= bytes_sent;
	}
    if (goodfile) {
        // send file https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/sendfile.2.html
        // https://blog.phusion.nl/2015/06/04/the-brokenness-of-the-sendfile-system-call/
        int n = filepath.length();
        char *chars = new char[n+1];
        strcpy(chars, filepath.c_str());

        int reqfd = open(chars, O_RDONLY);
        bytes_left = fsize;
        bytes_sent = sendfile(socket, reqfd, NULL, fsize);
        if(bytes_left < 0){
            cerr << "Errror sending file" << endl;
            return -1;
        }

        bytes_left -= bytes_sent;

        while (bytes_left > 0){
            DEBUG_PRINT("bytes_sent");
            bytes_sent = sendfile(socket, reqfd, NULL, fsize);
            bytes_left -= bytes_sent;
        }

        reqfile.close();
        DEBUG_PRINT("wrote header");
    }
                                         
    // tell to close the socket or not
    if (http11 && goodreq) {
        return 1;
    } else {
        return 0;
    }
}

/*

Purpose: Establish a new socket for a incoming conncetion and call functions to deal with
incoming message from socket
Parameters: pointer to a new nocket identifier
Return value: none

*/
void *new_connection(void *info) {
    struct timeval time;
    time.tv_sec = 40;
    
    struct arg_struct *args = (struct arg_struct *)info;
    string rootdir = args->arg1;
    cout << rootdir << "this is rootdir" << endl;
    int sock = args->arg2;
    
    int connection = 1;
	while(connection){
        
	    char req[MAXREQ] = {0};
	    int n = read(sock, req, MAXURI);
	    if (n < 0) {
	        cerr << "error on read!/n" << endl;
	    }
	    DEBUG_PRINT("MESSAGE RECIEVED: %s\n", req);
	    if (!handle_request(req, sock, rootdir)) { // if 0 (http1.0) close the socket
	        connection = 0;
	    }
        if(connections > 5){
            time.tv_sec = 20;
        }
        else if(connections > 10){
            time.tv_sec = 10;
        }
        cout << "this is the number of connections" << connections << endl;
        if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)(&time), sizeof(struct timeval)) < 0){
            cerr << "set sock options failing." << endl;
            perror("socket failing");
            if(errno == EFAULT){
                cerr << "sockfd" << endl;
            }
            else if(errno == EINVAL){
                cerr << "optlen value bad" << endl;
            }

        }
	}
	DEBUG_PRINT("Closing socket\n");
	close(sock);
   
    mtx.lock();
    conections--;
    mtx.unlock();
    return NULL;

}


/*

Purpose: Parse command line arguments, Create and bind socket to portnumber and listen
for connection requests. The function uses a while loop to listen for incoming connection requests
and when one is succesfully connected creates a thread for each new connection.
Parameters: none
Return Value: none

*/

int main(int argc, char** argv) {
    
    //literals
    int c, err, portnum, pflag, rflag = 0;
    char *rootdir;
    int sock_fd, new_sock, clientlen;
    struct sockaddr_in client_addr;
    
    //Parse command line
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
    //check err flag
    if (err || !pflag || !rflag) {
        perror("error on commandline");
    }
    
    DEBUG_PRINT("portnum %d, rootdir %s", portnum, rootdir);
    
 
    //Setup for socket
    clientlen = sizeof(client_addr);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);


    //set sock options for timeout
    // if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, ))
    
    //make sure sockfd is open
    if (sock_fd < 0) {
        printf("error opening socket\n");
        return -1;
    }
    
    //set sock structure
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(portnum);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Bind socket and make sure it is correct
    if (::bind(sock_fd, (struct sockaddr*) &myaddr, sizeof(myaddr)) < 0) {
        printf("error binding socket\n");
        return -1;
    }


    
    DEBUG_PRINT("opened and bound socket!\n");
    
    //listedn for upcoming conections
    if (listen(sock_fd,5) < 0) {
        perror("error on listen!\n");
        return -1;
    }

    //Have a while loop that wiats for incoming connections
    while (1) {
        new_sock = accept(sock_fd, (struct sockaddr *) &client_addr, (socklen_t*) &clientlen);
        DEBUG_PRINT("Connection found and accepted\n")
        if (new_sock < 0) {
            cerr << "error on accept!\n" << endl;
            return -1;
        }
       	
        mtx.lock();
        conections++;
        mtx.unlock();
        
        pthread_t new_thread;
        struct arg_struct args;
        args.arg1 = rootdir;
        args.arg2 = new_sock;
        
        if(pthread_create( &new_thread, NULL, new_connection, (void*)&args ) < 0){
        	cerr << "Thread Creation Failed" << endl;
        	return -1;
    	}

    }
    
    return 0;
}


