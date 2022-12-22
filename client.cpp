#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <bits/stdc++.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

#define PORT 3490       // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define INPUT_FILE_PATH "input_file.txt"
int source_port_number = 0;
string server_ip="";
int server_port_number = 0;
int socketfd=0;

typedef struct query{
    bool get;  // false => post
    string file_path;
    string host_name;
    int port_num_used;
    // string dest_ip_number;
    // int dest_port_number;
};
vector<query> queries;


void validate_read_line(int numArgs, vector<string> args){
    if(numArgs < 3){ //check number of arguments
        cout<<"Too few arguments, Try again."<<endl;
        exit(1);
    }

    if(numArgs > 4){ //check number of arguments
        cout<<"Too much arguments, Try again."<<endl;
        exit(1);
    }


    string command = args[0];
    string file_path = args[1];
    string host_name = args[2];
    string port_num_str = "";

    if(numArgs == 4) port_num_str=args[3];
    if(command != "client_get" || command != "client_post"){
        cout<<"Invalid command, make sure it is client_get or client_post, lowercase letters."<<endl;
        exit(1);
    } 

    if(port_num_str != ""){
        int temp_port_num = stoi(port_num_str);
        if(temp_port_num <= 1000) { //error or kernel reserved ports 
            cout<<"Wrong/unaccepted port number. "<<endl;
            exit(1);
        }
        // dest_port_number = temp_port_num;
    }
    


}








void process_query(query q){


    //formulate message
    string request = "";
    if(q.get) request = "GET "+q.file_path+" HTTP/1.1\r\n\r\n";
    else request = "POST "+q.file_path+" HTTP/1.1\r\n\r\n";

    //send http request
    if(send(socketfd,request.c_str(),request.length(),0) == -1){
        cout<<"error sending request"<<endl;
        exit(1);
    }
    
    if(q.get){
        //first chunk 
        char buff[CHUNK_SIZE];
        memset(buff,0,CHUNK_SIZE);
        int read_bytes = recv(socketfd,buff,CHUNK_SIZE,0);

        char c1 = buff[0];
        char c2 = buff[1];
        int ind = 2;
        int read_lines_to_read_header=2;
        string code_message="" +c1+c2;
        while (c1 != '\r' && c2!='\n' && ind<read_bytes)
        {
            c1=c2;
            c2=buff[ind];
            ind++;
            code_message = code_message+c2;
        }
        cout<<code_message<<endl;
        if(code_message!="HTTP/1.1 200 OK\r\n"){
            // we didn't receive the ok message
            cout<<code_message<<endl;
            exit(1);
        }

        string header_data ="";
        while (ind <read_bytes){
            header_data= header_data+buff[ind++];
        }

        int end_header_pos = header_data.find("\r\n\r\n");
        string headers = header_data.substr(0,end_header_pos);
        string body = header_data.substr(end_header_pos+5);

        char* headers_copy = strcpy(headers_copy,headers.c_str());
        vector<string> header_vec ;
        char* token;
        token = strtok(headers_copy, "\r\n"); 
        while (token != NULL)  
        {  
            char* header;
            strcpy(header,token);
            header_vec.push_back(header);
            token = strtok (NULL, "\r\n");  
        }  

        string size="";
        int size_int = -1;
        for(int i=0;i<header_vec.size();i++){
            vector<string> divided = get_words(header_vec[i]);
            if(divided[0]=="Content-Length:"){
                size = divided[1];
                size_int = stoi(size);
            }
        }


        memset(buff,0,CHUNK_SIZE);

        fstream myFile;
        string file_new_name = q.file_path;
        myFile.open (file_new_name, ios::binary);
        myFile.write(body.c_str(),body.length());
        int bytes_missing = size_int - body.length();
        while (bytes_missing>0)
        {
            char temp_buff[CHUNK_SIZE];
            int bytes = recv(socketfd,temp_buff,CHUNK_SIZE,0);
            bytes_missing-=bytes;
            myFile.write(temp_buff,bytes);
        }
        
        myFile.close();
        //write body to file
        // bytes still missing = size - body bytes
        // while loop
        //          keep recv until no other bytes to be read
        //          output to file
        // close all files
    }else{
        //open file 
        //get file size 
        // content_sent = file size
        //send ->"Content-Length: # \r\n\r\n"        
        // while(content length>0)
        //      read CHUNKMAX bytes from file (or less if content length<CHUNKMAX)
        //      send MAXCHUNK bytes you can send    ??? But, should I write whole buffer and depend that he will stop ? or write only bytes I wanna send
        //      content length -= MAXCHNK 
        //



    }   
}




int main(int argc, char** argv)
{ 
    int numArgs = argc-1;
    if(numArgs != 2 ){
        cout<<"wrong number of args"<<endl;
        exit(1);
    } else {
        string port_str = argv[2];
        int temp_port_num = stoi(port_str);
        if(temp_port_num <= 1000) { //error or kernel reserved ports 
            cout<<"Wrong/unaccepted port number. "<<endl;
            exit(1);
        }
        source_port_number=temp_port_num;

        if(argv[1]=="localhost") server_ip="127.0.0.1";
        else server_ip=argv[1];
        // server_ip = argv[1];
    }





    int sockfd;
    //create file descriptor for communication
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout<<"error creating socket"<<endl;
        exit(1);
    }
    //setup address struct
    struct sockaddr_in serv_addr =*(new sockaddr_in());
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port = htons(server_port_number);
    inet_aton(server_ip.c_str(), &(serv_اaddr.sin_addr));
    memset(&(serv_addr.sin_zero),'\0',8);

    //connect
    if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
        cout<<"error connecting"<<endl;
        exit(1);
    }
    socketfd = sockfd;



    fstream input_file;
    input_file.open(INPUT_FILE_PATH,ios::in);
    vector<query> v;
    if(input_file.is_open()){
        string line;
        int i=0;
        while (getline(input_file,line,'\n'))
        {
            vector<string> words = get_words(line);
            query q;
            validate_read_line(words.size(),words);            // validate line input
            if(words[0]=="client_get") q.get=true;
            else if(words[0]=="client_post") q.get=false;
            else {
                cout << "unknown command at line "+i <<endl;
                exit(1);
            }
            i++;
            q.file_path=words[1];
            q.host_name=words[2];
            if(words.size()==4){
                int temp_port_num = stoi(words[3]);
                if(temp_port_num <= 1000) { //error or kernel reserved ports 
                    cout<<"Wrong/unaccepted port number. "<<endl;
                    exit(1);
                }
                q.port_num_used=temp_port_num;
            }
            queries.push_back(q);
        }
    } else {
        cout<<"error opening file"<<endl;
        exit(1);
    }


    for(int i=0;i<queries.size();i++){
        //process query
        process_query(queries[i]);
    }



    





}