/* 

Created by Ian Squiers & Sean Cork on 1/30/19.

This program is an implementatin of a server side client that Handles HTTP requests. This
Program works by first opening and binding a socket then listening and accepting connections. This program
contains functions to make sure the HTTP request being sent is valid and returns then approriate messages.
This program was designed to only handle HTTP GET requests in HTTP/1.0 and HTTP/1.1. The program only supports the 
following file formats: .jpg, .html, .gif and .txt. The program works first by estabilishing
a new connection and creating a thread to parse and handle the HTTP request. The function that parses the request is used
to check if the HTTP request has the correct headers and is formated correctly. After 
determining the HTTP requests correctness. The program attempts to send the requested file to the client.
After determinign that everything is sent the program checks to see if the HTTP request, requested to keep 
the connection open or not. If the connection is to be closed the server closes the socket and waits for more
connections.

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
#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>

//Constants used to define easier printing statmenets

//max request buffer
const int MAXURI = 4000;
const int MAXREQ = 4000;

using namespace std;


//utils
static map<string, string> ftypes = {
    { ".gif", "image/gif"  },
    { ".jpg", "image/jpg"  },
    { ".txt", "text/plain" },
    { ".html", "text/html" }
};

struct arg_struct {
    string arg1;
    int arg2;
}args;

struct request_struct {
    int get = 0;
    string http_type = "";
    string filepath = "";
    int host = 0;
    int calive = 0;
    int cclose = 0;
    int done = 0;
    int cHTTP = 0;
};

void reset_info(request_struct &info) {
    info.get = 0;
    info.http_type = "";
    info.filepath = "";
    info.host = 0;
    info.calive = 0;
    info.cclose = 0;
    info.done = 0;
    info.cHTTP = 0;
}

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
    // cout << suffix << endl;
    map<string, string>::iterator find;
    find = ftypes.find(suffix);
    string filetype;

    //ifthe file type is not supported by our program raise a flag
    if(find == ftypes.end()){
    	filetype = "cant handle request";
    }
    else {
    	filetype = find->second;
    }
    return filetype;
}

/*

Purpose: Get the size of the file in the parameter
Parameter:filename
Return value: int that is the size of the file

*/

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

Purpose: Returns a string with the current date and time
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

Purpose: This function takes the message recieved from the socket and the socket number. First it determines
ifthe GET request is correctly formated and that the HTTP request is a GET request. It then parses the filepath of
the HTTP request. ifthe HTTP request is formatted correctly the function calls Generate Response which generates an
HTTP message to send. The function then attempts to send the message to the client. ifrequest is HTTP/1.0 it closes
the socket. Otherwise the socket is kept open.
Parameters: Char* msg is the buffer recieved from the socket. Int socket is the identifier for the socket number of the
request.
Return value: 1 ifhttp1.1 and good request, otherwise return 0.

*/

int handle_request(int socket, string rootdir, request_struct &rinfo) {
    
    //flags
    size_t fsize;
    int goodfile = 0;
    string filepath = rinfo.filepath;
    ifstream reqfile;
    int goodreq = 0;

    const char *header;
    
    // ifget is bad or neither http req
    if(rinfo.get && (rinfo.http_type.empty())) {
        header = (char*)"400 Bad Request\r\n";
    } 
    if(rinfo.http_type.back() == '1' && !(rinfo.cclose || rinfo.calive)) {
        header = (char*)"400 Bad Request\r\n";
    }
   
    //bad http version string
    if(!rinfo.cHTTP){
    	header = (char*)"400 Bad Request\r\n";
    }

    //if good request
    else {

    	//set flag
		goodreq = 1;

		//ifrootdir is not absolute set path to relatiuce directory

        if(rootdir[0] != '/'){
            char directory[100];
            if(getcwd(directory, sizeof(directory)) == NULL){
                cerr << "error getting current working directory" << endl;
            }
            rootdir = (string)directory + "/" + rootdir;
            // cout << rootdir << endl;
        }
        //if filepath is just a backline fetch index.html
        if(filepath.length() == 1 && filepath.compare("/") == 0){
            filepath = "/index.html";
            filepath = rootdir + filepath;
        }
        else {
            filepath = rootdir + filepath;
        }

        reqfile.open(filepath, ios::binary);
        if(errno == ENOENT || reqfile.fail()) { // file does not exist
            //DEBUG_PRINT("File does not exist");
            header = (char*)"404 Not Found\r\n";  
        } 
        else if(errno == EACCES) { // permission denied
            //DEBUG_PRINT("Failed read access");
            header = (char*)"403 Forbidden\r\n";
        }
        else if(!filetype(filepath).compare("cant handle request")) {
            //DEBUG_PRINT("Incompatable FileExtension");
            header = (char*)"404 Bad Request\r\n";
        }

        else {
            //DEBUG_PRINT("HERE 1");
            goodfile = 1;
            string type_of_file = filetype(filepath);
            string status = rinfo.http_type + " 200 OK\r\n";
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

    size_t bytes_sent;

    //send header info
    size_t bytes_left = strlen(header);
    // cout << "this is header" << header << endl;
    // cout << bytes_left << endl;
    bytes_sent = send(socket, header, strlen(header) ,0);
	if(bytes_left < 0){
		cerr << "Errror sending headers" << endl;
		return -1;
	}

	bytes_left -= bytes_sent;

	while (bytes_left > 0){
        // cout << bytes_left << endl;
		bytes_sent = send(socket, header, bytes_left, 0);
		bytes_left -= bytes_sent;
	}
    if(goodfile) {

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
            //DEBUG_PRINT("bytes_sent");
            bytes_sent = sendfile(socket, reqfd, NULL, fsize);
            bytes_left -= bytes_sent;
        }

        reqfile.close();
        //DEBUG_PRINT("wrote header");
    }
                                         
    // tell to close the socket or not
    if(rinfo.calive && goodreq) {
        //DEBUG_PRINT("KEEP ALIVE");
        return 1;

    } else {
        return 0;
    }
}

/*
 
Purpose: this function is a sinple debugging function that prints our struct that contains all of the information that
We need for our program:
Parameters: The struct that contains the relevant information that is needed.
Return Value: none
 
 */

void prints(request_struct &toprint) {
  cout << "get " << toprint.get << endl;
  cout << "http_type " << toprint.http_type << endl;
  cout << "filepath " << toprint.filepath << endl;
  cout << "host " << toprint.host << endl;
  cout << "calive " << toprint.calive << endl;
  cout << "cclose  " << toprint.cclose << endl;
  cout << "done  " << toprint.done << endl;
}

/*

Purpose: This function takes a line of an HTTP request and correctly identifies if an HTTP request is correct or not
Parameters: A char buffer that contains a line of the HTTP request and a sturct that contians the information regarding
each Header
Return value: none
 
 */
void tokenize_line(char* msg, request_struct &rinfo) {
	//copy char buffer

    char *request;
    char *rest = msg;
    request = strtok_r(rest, " ", &rest);
    int get = 0; // get line
    int con = 0; // connection line
    int pos = 0; // order of req words

    //while still tokens
    while(request != NULL) {
        cerr << "Processing token: " << request << endl;
        if(!strcmp("GET", request) && pos == 0){
            //DEBUG_PRINT("Reading GET line");
            get = 1;
            rinfo.get = 1;
        }
        //get filepath
        else if(pos == 1 && get) {
            rinfo.filepath = request;
            cerr << rinfo.filepath << " this is filepath" << endl;
        }
        if(!strncmp("HTTP/1.0", request, strlen("HTTP/1.0")) && pos == 2 && get) {
            rinfo.http_type = "HTTP/1.0";
            rinfo.cHTTP = 1;
            cerr << rinfo.http_type << " this is http_type" << endl;
        }
        else if(!strncmp("HTTP/1.1", request, strlen("HTTP/1.1")) && pos == 2 && get) {
            rinfo.http_type = "HTTP/1.1";
            cerr << rinfo.http_type << " this is http_type" << endl;
            rinfo.calive = 1;
            rinfo.cHTTP = 1;
        }
        else if(!strncmp("Connection:", request, strlen("Connection:"))) {
            //DEBUG_PRINT("Reading Connection line");
            con = 1;
        }
        else if(!strncmp("Keep-Alive", request, strlen("Keep-Alive")) && con) {
            rinfo.calive = 1;
        }
        else if(!strncmp("close", request, strlen("close")) && pos == 1 && con) {
            rinfo.cclose = 1;
        }
        else if(!strncmp("Host", request, strlen("Host")) && pos == 0) {
            rinfo.host = 1;
        }
        request = strtok_r(rest, " ", &rest);
        pos++;
    }
}

/*

Purpose: The purpose of this function is to tokenize the entire length of the HTTP message line by line
The function splits the HTTP request by carriage request making a token out of each line.
Parameters:A char buffer which contains the HTTP request and a sturct that contains all of the HTTP request information)
Return Value: none

*/

void tokenize_msg(char* msg, request_struct &rinfo) {


    //Check to see if message is complete
    if(strstr(msg, "\r\n\r\n") != NULL) {
        rinfo.done = 1;
    }
    char *request;
    char *rest = msg;

    //parse each line
    request = strtok_r(rest, "\r\n", &rest);
    while(request != NULL){
        cerr << "Processing line token: " << request << endl;
        tokenize_line(request, rinfo);
        request = strtok_r(rest, "\r\n", &rest);
    }
    if(!strlen(msg)) {
      rinfo.done = 1;
      return;
    }
}

/*

Purpose: Establish a new socket for a incoming conncetion and call functions to deal with
incoming message from socket.Set socketoptions for timing out request if HTTP/1.1 or HTTP request wishes
to keep connection open.
Parameters: pointer to a new nocket identifier
Return value: none

*/
void *new_connection(void *info) {

	//struct for setsockopt implementation
    struct timeval time;
    time.tv_sec = 40;
    
    struct arg_struct *args = (struct arg_struct *)info;

    string rootdir = args->arg1;
    if(rootdir.length() == 0){
        cout << "in here" << endl;
    	 char directory[100];
         if(getcwd(directory, sizeof(directory)) == NULL){
         	cerr << "error getting current working directory" << endl;
         }
         rootdir = (string)directory;
    }

    // cout << rootdir << "int rootdir" << endl;
    int sock = args->arg2;
    
    request_struct rinfo;

    //flag that tells us if we want to keep the connection open (HTTP/1.0, HTTP1.1);
    int connection = 1;
	while(connection){ 


		//while the HTTP request is still beign sent
        while(!rinfo.done) {
            char req[MAXREQ] = {0};
            int n = recv(sock, req, MAXURI, 0);
            //DEBUG_PRINT("MESSAGE RECIEVED: ->%s\n<-", req);

            if(n < 0) {
                cerr << "error on read!/n" << endl;
                continue;
            }
            if(strlen(req)) { // iflength of message >0
                tokenize_msg(req, rinfo);
                prints(rinfo);
            }
            else {
                //DEBUG_PRINT("message of length zero"); // right now we are just spinning ifwe dont close socket, constantly readigng \n
                connection = 0;
            }
        }
	    
	     //Tells us that we want to end the connection
	    if(!handle_request(sock, rootdir, rinfo) && connection == 1) { // if0 (http1.0) close the socket
	        connection = 0;
	    }
        reset_info(rinfo);

        //simnple heuristic
        if(connections > 5){
            time.tv_sec = 20;
        }
        else if(connections > 10){
            time.tv_sec = 10;
        }

        // cout << "Number of Connections " << connections << endl;

        if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)(&time), sizeof(struct timeval)) < 0){
            cerr << "set sock options failing." << endl;
            perror("socket 2failing");
            if(errno == EFAULT){
                cerr << "sockfd" << endl;
            }
            else if(errno == EINVAL){
                cerr << "optlen value bad" << endl;
            }

        }
	}
	//DEBUG_PRINT("Closing socket\n");
	close(sock);
   
    mtx.lock();
    connections--;
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
    string rootdir = "";
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
            //rflag = 1;
            rootdir = optarg;
            break;
        case '?':
            err = 1;
            break;
    }
    //check err flag
    if(err || !pflag || !rflag) {
        perror("error on commandline");
    }
    
    //DEBUG_PRINT("portnum %d, rootdir %s", portnum, rootdir);
    
 
    //Setup for socket
    clientlen = sizeof(client_addr);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);


    //set sock options for timeout
    // if(setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, ))
    
    //make sure sockfd is open
    if(sock_fd < 0) {
        printf("error opening socket\n");
        return -1;
    }
    
    //set sock structure
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(portnum);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Bind socket and make sure it is correct
    if(::bind(sock_fd, (struct sockaddr*) &myaddr, sizeof(myaddr)) < 0) {
        printf("error binding socket\n");
        return -1;
    }


    
    //DEBUG_PRINT("opened and bound socket!\n");
    
    //listedn for upcoming conections
    if(listen(sock_fd,20) < 0) {
        perror("error on listen!\n");
        return -1;
    }

    //Have a while loop that wiats for incoming connections
    while (1) {
        new_sock = accept(sock_fd, (struct sockaddr *) &client_addr, (socklen_t*) &clientlen);
        //DEBUG_PRINT("Connection found and accepted\n")
        if(new_sock < 0) {
            cerr << "error on accept!\n" << endl;
            return -1;
        }
       	
        mtx.lock();
        connections++;
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


