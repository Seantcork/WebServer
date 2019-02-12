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
#include <map>
#include <math.h>

using namespace std;
#define DEBUG_ME 0
#define DEBUG_PRINT(format, ...) if(DEBUG_ME) {\
printf("%s:%d -> " format "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__);\
fflush(stdout);}
static map<string, string> ftypes = { //utils
    { ".gif", "image/gif"  },
    { ".png", "image/png"  },
    { ".txt", "text/plain" },
    { ".html", "text/html" }
};
const int MAXURI = 255;
const int MAXREQ = 80; // good RoT for this?

const int BUFFERSIZE = 255;

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


char *generate_response(string http_type, string filepath, string rootdir) {
    DEBUG_PRINT("GENERATING RESPONSE");
    string response;
    string status;
    string ctype;
    string clen;
    string date;
    char* cresponse;
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
        file.open(filepath, std::ios::binary | std::ios_base::in);
        if (file.fail()) {
            //file does not exist 404
            DEBUG_PRINT("HERE");
            return (char*)"404 Not Found\n";
        }
    }
    file.seekg(0, file.end);
    int fsize = int(file.tellg());
    file.seekg(0, file.beg);

    
    cout << "FILESIZE: " << fsize << endl;
    
    if (fsize > MAXURI) {
        //handle splitting of response
    }
    
    vector<char> fdata(fsize);
    char *mydata = 0;
    mydata = new char[fsize];
    file.seekg(0, file.beg);
    


    //cout << file.tellg() << " laskjdf;lkasdjf;alsdjf;alskdf  " << mydata << endl;
    
    file.seekg(12,ios_base::cur);
    
    if (file.read(mydata, 100)) { //data successfully read
        // for (unsigned i = 0; i < fdata.size(); ++i){
        //        cout << fdata[i] << " ";
        // }
        string test = string(mydata);
        cout << strlen(mydata) << " DATA READ  " << test << endl;
        
        status = http_type + " 200 OK\r\n";
        date = get_date(); 
        ctype = "Content-Type: " + type_of_file + "\r\n";
        cerr << type_of_file << endl;
        
        clen = "Content-Length: " + to_string(fsize) + "\r\n";
        response = status + date + ctype + clen + "\r\n"; // + fdata.data() + "\r\n";
        

        strcpy(cresponse, response.c_str());

        // for (unsigned i = 0; i < response.length(); ++i){
        //         cout << *cresponse;
        //         *cresponse++;
        // }


        int n = response.length();
        char *char_array = new char[fsize + response.length()];
        //strcpy(char_array, mydata);
        memcpy(char_array, mydata, fsize + response.length());
        cout << "heeeee" << endl;
        memcpy(char_array, cresponse, response.length());

        // for (unsigned i = 0; i < fsize; ++i){
        //         cout << *char_array;
        //         *char_array++;
        // }
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

int main(int argc, char** argv) {
    
    char* cat = generate_response("HTTP/1.1","IMG_7290.png","/Users/seancork/Desktop/");

    for(int i =0; i < 84255; i++){
        cout << *cat;
        *cat++;
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
