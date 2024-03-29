#include <cstdlib>
#include <iostream>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <bits/stdc++.h>
#include "../common/common.h"
#include <arpa/inet.h>
#include <sstream>
#include <vector>
#include "../common/myHTTP.cpp"
#include <sys/wait.h>

using namespace std;
#define INPUT_FILE_PATH "input_file.txt"
string server_ip="";
short server_port_number = 0;
int socketfd=0;

typedef struct query{
    bool get;  // false => post
    string file_path;
    string host_name;
    int port_num_used;

} query;
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
    string port_num_str;

    if(numArgs == 4) port_num_str=args[3];
    if(command != "client_get" && command != "client_post"){
        cout<<"Invalid command, make sure it is client_get or client_post, lowercase letters." << command <<'|'<<endl;
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





//              GET
//write body to file
// bytes still missing = size - body bytes
// while loop
//          keep recv until no other bytes to be read
//          output to file
// close all files


void process_get(query q){
    string request = "GET /"+q.file_path+" HTTP/1.1\r\n\r\n";

    if(send(socketfd,request.c_str(),request.size(),0) == -1){
        cout<<"error sending request"<<endl;
        exit(1);
    }
    //first chunk
    char buff[CHUNK_SIZE];
    memset(buff,0,CHUNK_SIZE);
    int read_bytes = recv(socketfd,buff,CHUNK_SIZE,0);
    cout << buff <<'\n';
    myHTTP parser;
    if(!parser.parseHTTP(buff,read_bytes))
    {
        cout<<"error parsing."<<"\n";
        exit(1);
    }
    string code_message = parser.headers_lines.at(0);
    if(parser.argument_or_code!="200"){
        // we didn't receive the ok message
        cout<<"ERROR READING OK MESSAGE"<<"\n";
        cout<<code_message<<endl;
        exit(1);
    }
    int length = parser.content_length;
    char* body_start = parser.body_start;

    fstream myFile;
    string file_new_name = "out_" + q.file_path;
    myFile.open (file_new_name, ofstream::binary | ofstream::out | ofstream::trunc);
    myFile.write(body_start,parser.body_size);
    int bytes_missing = length - parser.body_size;
    // build the timeval struct to call select
    timeval time_out{};
    time_out.tv_sec = DURATION_BEFORE_TIME_OUT;
    time_out.tv_usec = 0;
    // set up the parameters for select
    fd_set sever_input;
    FD_ZERO(&sever_input);
    FD_SET(socketfd, &sever_input);
    int select_returned = -1;
    // wait till time_out or something is there to read
    while (bytes_missing>0)
    {
        select_returned = select(socketfd + 1, &sever_input, nullptr, nullptr, &time_out);
        if(select_returned == 0){
            perror("sever timed out");
            break;
        }
        if(select_returned == -1){
            perror("select err");
            break;
        }
        char temp_buff[CHUNK_SIZE];
        memset(temp_buff,'\0',CHUNK_SIZE);
        int bytes = recv(socketfd,temp_buff,CHUNK_SIZE,0);
        bytes_missing-=bytes;
        if(bytes==0) break;
        myFile.write(temp_buff,bytes);
    }
    myFile.close();

}



//          POST
//open file
//get file size
// content_sent = file size
//send ->"Content-Length: # \r\n\r\n"
// while(content length>0)
//      read CHUNKMAX bytes from file (or less if content length<CHUNKMAX)
//      send MAXCHUNK bytes you can send    ??? But, should I write whole buffer and depend that he will stop ? or write only bytes I wanna send
//      content length -= MAXCHNK


void process_post(query q){
    string request= "POST /"+q.file_path+" HTTP/1.1\r\n";
    ifstream in(q.file_path, ifstream::ate | ifstream::binary);
    // get file size
    int file_size = in.tellg();
    in.seekg(0, std::ios_base::beg);
    char *chunk = (char *)(malloc(CHUNK_SIZE));
    memset(chunk, 0, CHUNK_SIZE);

    if (in.fail())
    {
        cout<<"error opening file in post\n";
        in.close();
        return;
    }
    std::vector<std::string> request_headers;
    request_headers.push_back(request);
    request_headers.push_back("Content-Length: "+ to_string(file_size));
    request_headers.emplace_back("\r\n");
    request_headers.emplace_back("\r\n");

    char* data_pointer_in_chunk_start = chunk;
    char* data_pointer_in_chunk_end = strscpy(chunk,request_headers);


    //write file data onto chunks and keep sending chunks till the whole file is sent

    //sudo:
    //-----
    //write up to the current chunksize
    //send it
    //while(file not fully sent)
        //reset the buffer for good measures
        //read chunk from file into buffer
        //keep writing till finished sending the chunk while()
    

    int file_bytes_sent = 0;
    //used_chunk_space
    int used_space = data_pointer_in_chunk_end - data_pointer_in_chunk_start;
    int actual_chunk_size = min(CHUNK_SIZE - used_space, file_size - file_bytes_sent);
    in.read(chunk + used_space,actual_chunk_size);
    int chunk_bytes_sent = 0;
    while (chunk_bytes_sent<actual_chunk_size+used_space)
    {
        int actual_chunk_bytes_sent = write(socketfd,chunk+chunk_bytes_sent,actual_chunk_size+used_space-chunk_bytes_sent);
        chunk_bytes_sent += actual_chunk_bytes_sent;
    }
    file_bytes_sent+=actual_chunk_size;


    while (file_bytes_sent < file_size && !in.eof())
    {
        int actual_chunk_size = min(CHUNK_SIZE, file_size - file_bytes_sent);
        memset(chunk, '\0', CHUNK_SIZE);
        in.read(chunk,actual_chunk_size);
        chunk_bytes_sent = 0;
        while (chunk_bytes_sent<actual_chunk_size)
        {
            int actual_chunk_bytes_sent = write(socketfd,chunk+chunk_bytes_sent,actual_chunk_size-chunk_bytes_sent);
            chunk_bytes_sent += actual_chunk_bytes_sent;
        }
        file_bytes_sent+=actual_chunk_size;
    }

    // free resources
    in.close();
    memset(chunk, '\0', CHUNK_SIZE);
    int response_size = read(socketfd,chunk,CHUNK_SIZE);
    string rsp =  string(chunk).substr(0,response_size);
    cout << rsp <<"\n";


    free(chunk);
}


void process_query(query q){
    if(q.get){
        process_get(q);
    }else{
        process_post(q);
    }   
}




int main(int argc, char** argv)
{ 
    // the number of args in c contains ./client used so we need args -1
    int numArgs = argc-1;

    //validate number of args
    if(numArgs != 2 ){
        cout<<"wrong number of args"<<endl;
        exit(1);
    } else {
        string port_str(argv[2]);
        int temp_port_num = std::stoi(port_str);
        if(temp_port_num <= 1000) { //error or kernel reserved ports 
            cout<<"Wrong/unaccepted port number. "<<endl;
            exit(1);
        }
        server_port_number=temp_port_num;
        server_ip=argv[1];
    }

    int sockfd;
    //create file descriptor for communication
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout<<"error creating socket"<<endl;
        exit(1);
    }

    //setup address struct
    struct sockaddr_in serv_addr{};

    serv_addr.sin_family=AF_INET; //family
    if(server_ip=="localhost"){
        serv_addr.sin_addr.s_addr = INADDR_ANY;  //local ip address bytes
    }
    else{
       inet_aton(server_ip.c_str(), &(serv_addr.sin_addr)); //convert from dot format to bytes
    }
    serv_addr.sin_port = htons(server_port_number); //converting port number to network byte order
    memset(&(serv_addr.sin_zero),'\0',8);           //setting to zeros

    cout<<server_ip<<"\n"<<server_port_number<<endl;

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
            if(q.file_path[0]=='/'){
                q.file_path = q.file_path.substr(1,q.file_path.size());
            }

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