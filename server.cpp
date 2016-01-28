#include "cs360utils.h"
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <sstream>
#define SOCKET_ERROR        -1
#define BUFFER_SIZE         1024
#define QUEUE_SIZE          5

//These are the messages that I will be using
#define STAT_200            "200 OK\r\n"
#define STAT_404            "404 Not Found\r\n"

// Type of files and other constants
#define F_HTML              "Content-Type: text/html\r\n"
#define F_JPG               "Content-Type: image/jpg\r\n"
#define F_GIF               "Content-Type: image/gif\r\n"
#define F_TXT               "Content-Type: text/plain\r\n"
#define MIME                "MIME-Version:1.0\r\n"
#define CONTENT             "Content-Length: %d\r\n\r\n"

// Define my magic numbers
#define PATH_LENGTH 36
#define CONTENT_LENGTH 50

using namespace std;

string parseRequest(int source) {

    // Grab the headers from the request

    vector<char*> headers;
    GetHeaderLines(headers, source,0);

    string request = "";

    for(int i = 0; i < headers.size(); i++) {
        
        cout << headers[i] << endl;
        string type(headers[i]);
        int index;

        // Find the type of request
        if((index = type.find("HTTP_GET")) >= 0) {
            cout << "Found page at index " << index << endl;
            int sindex, eindex;
            sindex = type.find(" ") + 1;
            eindex = type.find(" ", sindex);
            request = type.substr(sindex, eindex - sindex);
            

            cout << request << endl;
            return request;
        }
    }
    return STAT_404;
}


string removeDash(string parsedHeaders){

    while( parsedHeaders[parsedHeaders.length()-1] == '/' ) {

            parsedHeaders = parsedHeaders.substr(0, parsedHeaders.length() - 1);
        }
        return parsedHeaders;
}

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    char pBuffer[BUFFER_SIZE];
    int nHostPort;


    // parse input params
    if(argc != 3) {
        printf("\nUsage: server host-port directory\n");
        return 0;
    }

    nHostPort = atoi(argv[1]);
    string rootDirectory = argv[2];


    printf("\nStarting server");
    
    printf("\nMaking socket");
    /* make a socket */
    hServerSocket=socket(AF_INET,SOCK_STREAM,0);
    
    if(hServerSocket == SOCKET_ERROR) {
        printf("\nCould not make a socket\n");
        return 0;
    }
    
    // Add code given to allow for port resusal.
    int optval = 1;
    setsockopt (hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    /* fill address struct */
    Address.sin_addr.s_addr=INADDR_ANY;
    Address.sin_port=htons(nHostPort);
    Address.sin_family=AF_INET;
    
    printf("\nBinding to port %d\n",nHostPort);
    
    /* bind to a port */
    if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address)) 
            == SOCKET_ERROR) {
        printf("\nCould not connect to host\n");
        return 0;
    }
    /*  get port number */
    getsockname(hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
    printf("opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port));
    
    printf("Server\n\
           sin_family        = %d\n\
           sin_addr.s_addr   = %d\n\
           sin_port          = %d\n"
           , Address.sin_family
           , Address.sin_addr.s_addr
           , ntohs(Address.sin_port)
           );
    
    
    printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR) {

        printf("\nCould not listen\n");
        return 0;
    }


    for(;;) {

        bool correctPath, isDirectory = false;
       
        
        char *response = (char *) malloc(sizeof(char) * 1024);
        strcat(response,"HTTP/1.1 ");

        printf("\nWaiting for a connection\n");
        /* get the connected socket */
        hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);

        printf("\nGot a connection from %X (%d)\n", Address.sin_addr.s_addr, ntohs(Address.sin_port));

        // parse the headers to see the request

        string parsedHeaders = parseRequest(hSocket);

        // get rid of dash
        parsedHeaders = removeDash(parsedHeaders);

        char clientPath[PATH_LENGTH];
        strcpy(clientPath,parsedHeaders.c_str());

        //Check for invalid response
        if (parsedHeaders == STAT_404) {
            cout << "Invalid response, No GET request\n";
        }

        
        struct stat filestat;
        char fullPath[PATH_LENGTH];
        strcpy(fullPath,rootDirectory.c_str());

        strcat(fullPath,parsedHeaders.c_str());
        
        if(stat(fullPath, &filestat)) {
            cout <<"\nThe file path does not exist\n";
        }
        else if(S_ISREG(filestat.st_mode)) {

            char * fileExtension = strrchr(clientPath, '.'); //extension of file path

            if(strcmp(fileExtension,".html") == 0) {

                strcat(response, STAT_200);
                strcat(response,MIME);
                strcat(response, F_HTML);
                char contentLength [CONTENT_LENGTH];
                sprintf(contentLength, CONTENT, (int)filestat.st_size);
                strcat(response, contentLength);
                correctPath = true;
            }
            else if(strcmp(fileExtension,".txt") == 0) {
                
                strcat(response, STAT_200);
                strcat(response,MIME);
                strcat(response, F_TXT);
                char contentLength [CONTENT_LENGTH];
                sprintf(contentLength, CONTENT, (int)filestat.st_size);
                strcat(response, contentLength);
                correctPath = true;
            }
             else if(strcmp(fileExtension,".jpg") == 0) {

                strcat(response, STAT_200);
                strcat(response,MIME);
                strcat(response, F_JPG);
                char contentLength [CONTENT_LENGTH];
                sprintf(contentLength, CONTENT, (int)filestat.st_size);
                strcat(response, contentLength);
                correctPath = true;
            }

            else if(strcmp(fileExtension,".gif") == 0) {

                strcat(response, STAT_200);
                strcat(response,MIME);
                strcat(response, F_GIF);
                char contentLength [CONTENT_LENGTH];
                sprintf(contentLength, CONTENT, (int)filestat.st_size);
                strcat(response, contentLength);
                correctPath = true;
            }

        }
        // After determining if there is a directory we need to determine if there exists a index.html
        else if(S_ISDIR(filestat.st_mode)) {

            correctPath = true;
            struct stat indexFile;
            char indexPath[PATH_LENGTH];
            strcpy(indexPath,fullPath);
            strcat(indexPath,"/index.html");

            // If there isn't a index.html make this a directory
            if(stat(indexPath, &indexFile)) {

                isDirectory = true;
                correctPath = false;
               
                strcpy(fullPath,indexPath);
            }
            else {

                strcat(response, STAT_200);
                strcat(response, MIME);
                strcat(response, F_HTML);
                char contentLength [CONTENT_LENGTH];
                sprintf(contentLength, CONTENT, (int)indexFile.st_size);
                strcat(response, contentLength);
                strcpy(fullPath,indexPath);
            }
        }

        if(correctPath) {

            FILE *file = fopen(fullPath,"r");
            write(hSocket, response, strlen(response));
            bzero((char*) &response, sizeof(response));

            // Write the This will send bytes until eof
            for(;;) {

                unsigned char buffer[256] = {0}; 
                int bytesRead = fread(buffer, 1, 256, file);

                if(bytesRead > 0) {
                    
                    write(hSocket, buffer, bytesRead);
                }
               
                if(bytesRead < 256) {
                    if(feof(file)) {
                        cout << "At the end of the file.\n";
                    }
                    if(ferror(file)) {
                        cout << "Error reading the file to the socket.\n";
                    }
                    break; // ends infinite loop
                }
            }
        }
        
        else if(isDirectory) {

                // use code from STAT in oder to display the directory
                cout << "At a directory\n";
                vector<string> subdirList;
                DIR *dir;
                struct dirent *dirEntries;
                string directoryPath = fullPath;

                if((dir = opendir(directoryPath.substr(0, directoryPath.length() - 11).c_str())) != NULL) {
                    while((dirEntries = readdir(dir)) != NULL) {
                        subdirList.push_back((string) dirEntries->d_name);
                    }
                    closedir(dir);
                }

                // make html for the directory listing
                std::stringstream ss;
                ss << "<!DOCTYPE html><html><head><title> Directory listing for ";
                ss << clientPath;
                ss << "</title></head><body>";
                ss << "<h1> Directory for ";
                ss << clientPath;
                ss << "</h1><div>";

                string relativePath = clientPath;
                
                for(int i = 0; i < subdirList.size(); i++) {
                    if(subdirList[i] == ".." || subdirList[i] == ".") {
                        continue;
                    }

                    ss << "<li><a href=\"";
                    ss << relativePath.substr(1, relativePath.length() - 1);
                    ss << "/";
                    ss << subdirList[i];
                    ss << "\">";
                    ss << subdirList[i];
                    ss << "</a></li>";
                }

                ss << "</div></body></html>";

                string directoryHTML = ss.str();
                strcat(response,STAT_200);
                strcat(response,MIME);
                strcat(response,F_HTML);
                char contentLength [CONTENT_LENGTH];
                sprintf(contentLength, CONTENT, (int)directoryHTML.length());
                strcat(response, contentLength);


                write(hSocket, response, strlen(response));
                write(hSocket, directoryHTML.c_str(), directoryHTML.length());
                bzero((char*) &response, sizeof(response));

        }else {

            stringstream ss;
            ss << "<html><head><title>404 - NOT FOUND</title></head>";
            ss << "<body><h1>404 - NOT FOUND</h1></body></html>";

            string errorHTML = ss.str();

            strcat(response, STAT_404);
            strcat(response, F_HTML);

            char contentLength [CONTENT_LENGTH];

            sprintf(contentLength, CONTENT, (int)errorHTML.length());
            strcat(response, contentLength);

            write(hSocket, response, strlen(response));
            bzero((char*) &response, sizeof(response));         
            write(hSocket, errorHTML.c_str(), errorHTML.length());
        }

        /* close socket */
        printf("\nClosing the socket");

        if(close(hSocket) == SOCKET_ERROR){
             printf("\nCould not close socket\n");
             return 0;
        }
    }
}
