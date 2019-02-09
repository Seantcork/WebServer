//
//  test.cpp
//  
//
//  Created by Ian Squiers on 2/6/19.
//

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

using namespace std;

const int BUFFERSIZE = 255;

string get_date() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer,80,"Date: %a, %d %b %G %T %T\n",timeinfo);
    string tstring = buffer;
    return tstring;
}

int main(int argc, char** argv) {
    
    //
    string http_type = "1.1";
    //
    
    string status = "HTTP/" + http_type + " 200 OK\n";
    string ctype = "Content-Type:\n";
    string clen = "Content-Length:\n";
    
    char filepath[] = "test.txt";
    
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

    return 0;
    
//
//    char buffer[255];
//
//    cout << "helloooo" << endl;
//    char filepath[] = "test.txt";
//    ifstream infile(filepath);
//
//    struct stat filestatus;
//    stat(filepath, &filestatus);
//    int fsize = filestatus.st_size;
//    cout <<"filesize" << fsize<<endl;
//
//
//    if (infile.is_open()) {
//        cout << "Hello";
//        buffer << infile;
//        cout << buffer;
//    } else {
//        cout << "error";
//    }
//    cout << "blah";
}
