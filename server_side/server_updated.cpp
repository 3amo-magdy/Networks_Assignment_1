/**
* while true: do
    Listen for connections
    Accept new connection from incoming client and delegate it to worker thread/process
    Parse HTTP/1.1 request and determine the command (GET or POST)
    Determine if target file exists (in case of GET) and return error otherwise
    Transmit contents of file (reads from the file and writes on the socket) (in case of GET)
    Wait for new requests (persistent connection)
    Close if connection timed out
endwhile

server: a forever running thread:
---------------------------------
map<fd -> last_active_ts>
int active_connections;
while(true)
    new_fd = blocking_accept()
    start new [worker] thread to handle new_fd, give him (&active_connections, new_fd)
-----------------------------------------------------------------------------------------
worker (int* active_connections, fd):
--------------------------------------
while(true):
    //critical section:
    lock()
        time = compute_time(active_connections)
    unlock()


    can_read = select(..,.., time)

    //check for select error (-1)
    if(can_read == -1):
        err & return

    //time_out (select returns with 0 when [time] has passed and nothing was there to read)
    if(can_Read == 0):
        //critical section:
        lock()
            active_connections* --;
        unlock()
        close(fd) or shut_down(fd) //not sure

    //we are now sure there's something to read hence there's no blocking here:
    data = read(fd,..)
    parse and build response:
    send(fd,response)
*/

#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <sstream>
#include <vector>
#include <fstream>
#include <cstring>
#include "../common/common.h"
#include "../common/myHTTP.cpp"
#include <iostream>
#define MAX_CLIENTS 20           // max allowed number of active clients
#define MAX_ONE_CLIENT_TIME_OUT 40.0 // the time_out duration if there's only one client connected, in seconds
#define MIN_ONE_CLIENT_TIME_OUT 8.0 // the time_out duration if there're MAX_CLIENTS connected, in seconds
#define CHUNK_SIZE 1024
static const std::string error_code = "HTTP/1.1 500 Internal Server Error\r\n";
static const std::string success_code = "HTTP/1.1 200 OK\r\n";
static const std::string clength_header = "Content-Length: ";
static const std::string welcome_msg("<html><body><h1>welcome to our server!</h1> - Use GET [file_name] [http_version]  to retrieve a file.<br>  - Use POST [file_name] [http_version]  to upload a file.</body></html>\r\n");
static const std::string empty_line = "\r\n";
static const std::string open_fail_err = "failed to open the file\r\n";
static const std::string un_supported_err = "un supported command\r\n";
static const std::string parse_err = "parsing error | invalid http request\r\n";
static const std::string read_err = "reading error in worker thread\r\n";
static const std::string fwrite_err = "file writing error in worker thread\r\n";

static const std::string file_received = "the file's chunk has been uploaded to the server successfully\r\n";

static int PORT = 6116;               // the default port to bind & listen on



/**
 * computes the duration before time_out based on the number of concurrent connections
 * @param n the number of active connections
 * @return the duration in seconds (ex: 4.324 s)
 */
static float compute_time_out(int n)
{
    // return ONE_CLIENT_TIME_OUT / n;
    return (MAX_CLIENTS - n)*((MAX_ONE_CLIENT_TIME_OUT - MIN_ONE_CLIENT_TIME_OUT) / (MAX_CLIENTS - 1));

}

int min(int a, int b)
{
    if (a <= b)
        return a;
    return b;
}

void send_open_fail_err(int fd)
{
    // send open_fail error
    std::vector<std::string> response;
    response.push_back(error_code);
    response.push_back(empty_line);
    response.push_back(open_fail_err);
    send_onto_socket(response, fd);
}
void send_reading_error(int fd)
{
    std::vector<std::string> response;
    response.push_back(error_code);
    response.push_back(empty_line);
    response.push_back(read_err);
    send_onto_socket(response, fd);
}
void send_writing_error(int fd)
{
    std::vector<std::string> response;
    response.push_back(error_code);
    response.push_back(empty_line);
    response.push_back(fwrite_err);
    send_onto_socket(response, fd);
}

void send_parse_err(int fd)
{
    std::vector<std::string> response;
    response.push_back(error_code);
    response.push_back(empty_line);
    response.push_back(parse_err);
    send_onto_socket(response, fd);
}


void handle_get_request(std::string file_path, int fd){
    std::ifstream in(file_path, std::ifstream::ate | std::ifstream::binary);
    // get file size
    int file_size = in.tellg();
    in.seekg(0, std::ios_base::beg);

    char *chunk = (char *)(malloc(CHUNK_SIZE));
    if (in.fail())
    {
        send_open_fail_err(fd);
        in.close();
        return;
    }
    std::vector<std::string> response_headers;
    //append success code
    response_headers.push_back(success_code);

    //append Content-Length header token
    response_headers.push_back(clength_header);

    // write content-length value:

    std::string body_size_str = std::to_string(file_size);
    response_headers.push_back(body_size_str);
    // write an empty line at end of header
    response_headers.push_back(empty_line);
    // write an empty line at end of header section
    response_headers.push_back(empty_line);
    
    char* data_pointer_in_chunk_end = strscpy(chunk,response_headers);
    char* data_pointer_in_chunk_start = chunk;

    bool chunk_should_be_empty = false;

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

    int used_space = data_pointer_in_chunk_end - data_pointer_in_chunk_start;

    int actual_chunk_size = min(CHUNK_SIZE - used_space, file_size - file_bytes_sent);
    // memset(chunk, 0, CHUNK_SIZE);
    in.read(chunk + used_space,actual_chunk_size);
    int chunk_bytes_sent = 0;
    while (chunk_bytes_sent<actual_chunk_size+used_space)
    {
        int actual_chunk_bytes_sent = write(fd,chunk+chunk_bytes_sent,actual_chunk_size+used_space-chunk_bytes_sent);
        chunk_bytes_sent += actual_chunk_bytes_sent;
    }
    file_bytes_sent+=actual_chunk_size;


    while (file_bytes_sent < file_size)
    {
        int actual_chunk_size = min(CHUNK_SIZE, file_size - file_bytes_sent);
        memset(chunk, 0, CHUNK_SIZE);
        in.read(chunk,actual_chunk_size);
        int chunk_bytes_sent = 0;
        while (chunk_bytes_sent<actual_chunk_size)
        {
            int actual_chunk_bytes_sent = write(fd,chunk+chunk_bytes_sent,actual_chunk_size-chunk_bytes_sent);
            if(actual_chunk_bytes_sent==0){
                break;
            }
            chunk_bytes_sent += actual_chunk_bytes_sent;
        }
        file_bytes_sent+=actual_chunk_size;
    }
    
    // free resources
    in.close();
    free(chunk);
}

int worker(int *active_connections, int fd, std::mutex *lock)
{
    std::string command;
    std::string file_path;

    while (true)
    {
        // critical section - on resource: active_connections
        // compute the duration based on the heuristic
        float duration;
        (lock)->lock();
        duration = compute_time_out(*active_connections);
        std::cout<<"fd "<<fd<<" has connection duration:"<<duration<<" s\n";
        (lock)->unlock();
        // build the timeval struct to call select
        timeval time_out{};
        time_out.tv_sec = (int)duration;
        time_out.tv_usec = (long)((duration - (int)duration) * 1000) * 1000;
        // set up the parameters for select
        fd_set input;
        FD_ZERO(&input);
        FD_SET(fd, &input);
        int can_read = -1;
        // wait till time_out or something is there to read
        can_read = select(fd + 1, &input, nullptr, nullptr, &time_out);
        if (can_read == -1)
        {
            perror("select err in worker");
            continue;
            // exit(1);
        }
        // if time_out occurred:
        else if (can_read == 0)
        {
            std::cout<<"fd "<< fd <<" has timed out !\n";
            lock->lock();
            *active_connections = *active_connections - 1;
            lock->unlock();
            close(fd);
            return 0;
        }
        // illegal state - must never happen -:
        if (!FD_ISSET(fd, &input))
        {
            std::cout << ("illegal state!\n");
            return 0 ;
        }
        // there's something to read:
        // read the chunk (unblocking after select()):
        char *buffer = (char *)malloc(CHUNK_SIZE);
        int buffer_offset = 0;
        int buffer_size = read(fd, buffer, CHUNK_SIZE);
        if(buffer_size==-1){
            perror("buffer error");
            return 0;
        }
        if (buffer_size == 0)
        {
            // close connection
            lock->lock();
            *active_connections = *active_connections - 1;
            lock->unlock();
            close(fd);
            return 0;
        }
        if (buffer_size == -1)
        {
            // perror("buffer reading err | worker");
            send_reading_error(fd);
            continue;
        }
        // print all what was received:
        std::cout << buffer << '\n';

        // parse the chunk into -> request, file_path, content_length(in case of post)
        std::string request = "";
        myHTTP parser;
        if(!parser.parseHTTP(buffer, buffer_size)){
            // send open_fail error
            send_parse_err(fd);
            continue;
        }
        command = parser.command_or_httpversion;
        file_path = parser.argument_or_code;
        if (file_path == "/")
        {
            // std::cout<<"welcome !"<<"\n";
            // allocate the in-memory chunk to be sent in socket
            std::vector<std::string> msgs{};
            // write success_code
            msgs.push_back(success_code);
            // write content-length header_name:
            msgs.push_back(clength_header);
            // write content-length header_value:
            int body_size = welcome_msg.size();
            std::string body_size_str = std::to_string(body_size);
            msgs.push_back(body_size_str);
            // write an empty line at end of header
            msgs.push_back(empty_line);
            // write an empty line at end of header section
            msgs.push_back(empty_line);
            //send welcoming msg
            msgs.push_back(welcome_msg);
            send_onto_socket(msgs,fd);
            continue;
        }
        if(file_path[0]=='/'){
            file_path = file_path.substr(1,file_path.size());
        }
        if (command == "GET")
        {
            handle_get_request(file_path,fd);
        }
        else if (command == "POST")
        {
            std::fstream myFile(file_path, std::ofstream::binary |std::ofstream::trunc|std::ofstream::out );
            myFile.write(parser.body_start,parser.body_size);
            int bytes_missing = parser.content_length - parser.body_size;
            // cout<<"\nBYTES MISSING "<<bytes_missing<<"\n";
            // build the timeval struct to call select
            timeval time_out_duration{};
            time_out_duration.tv_sec = DURATION_BEFORE_TIME_OUT+10 ;
            time_out_duration.tv_usec = 1000;
            // set up the parameters for select
            fd_set client_input;
            FD_ZERO(&client_input);
            FD_SET(fd, &client_input);
            int select_returned = -1;
            bool failed = false;
            // wait till time_out or something is there to read
            char* temp_buff = (char*) malloc(CHUNK_SIZE);
            while (bytes_missing > 0)
            {
                time_out_duration.tv_sec = DURATION_BEFORE_TIME_OUT;
                time_out_duration.tv_usec = 1000;
                select_returned = select(fd + 1, &client_input, nullptr, nullptr, &time_out);
                if(select_returned == 0){
                    failed = true;
                    std::cout << "select returned 0 time out !";
                    break;
                }
                if(select_returned == -1){
                    failed = true;
                    std::cout << "select returned -1 !";
                    break;
                }
                if(!FD_ISSET(fd,&client_input)){
                    std::cout<<"AUUUUUUUUUGH WTF"<<"\n";
                }
                memset(temp_buff,'\0',CHUNK_SIZE);
                int bytes = read(fd,temp_buff,min(CHUNK_SIZE,bytes_missing));
                if(bytes == -1){
                    perror("client disconnected");
                    return 0;
                }
                bytes_missing-=bytes;
                if(bytes==0){
                    std::cout << "read returned 0!";  
                    break;
                }
                myFile.write(temp_buff,bytes);
            }
            myFile.close();
            free(temp_buff);
            if(failed){
                send_reading_error(fd);
                continue;
            }            
            //build & send response
            std::vector<std::string> response;
            response.push_back(success_code);
            response.push_back(empty_line);
            response.push_back(file_received);
            send_onto_socket(response, fd);
        }
        else
        {
            //"unsupported command"
            std::vector<std::string> response;
            response.push_back(error_code);
            response.push_back(empty_line);
            response.push_back(un_supported_err);
            send_onto_socket(response, fd);
        }
    }
}

int main(int argc, char** argv){
    if(argc<1){
        std::cout << "type in atleast one argumet."<<"\n";
    }
    PORT = std::stoi(argv[1]);
    std::cout<< PORT <<"\n";
    int active_connections = 0;
    std::mutex lock;
    int listener; // listening socket descriptor

    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int addrlen;

    // create the server socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket err");
        exit(1);
    }
    // allow the program to reuse the port
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt err");
        exit(1);
    }
    sockaddr_in server_addr_in{}; // server address
    sockaddr_in client_addr_in{}; // client address

    // build the addr struct
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_addr.s_addr = INADDR_ANY;
    server_addr_in.sin_port = htons(PORT);
    memset((&(server_addr_in.sin_zero)), '\0', sizeof(server_addr_in.sin_zero));

    // bind the socket to the ip & port
    if (bind(listener, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) == -1)
    {
        perror("bind err");
        exit(1);
    }
    // make the socket start the socket listening to client connections
    if (listen(listener, MAX_CLIENTS) == -1)
    {
        perror("listen err");
        exit(1);
    }
    // the server loop (forever running)
    while (true)
    {
        // handle new connections
        addrlen = sizeof(client_addr_in);
        int new_fd = -1;
        new_fd = accept(listener, (struct sockaddr *)&client_addr_in, (socklen_t *)(&addrlen));
        if (new_fd == -1)
        {
            perror("accept err");
        }
        else
        {
            printf("new connection from %s on fd %d\n", inet_ntoa(client_addr_in.sin_addr), new_fd);
            active_connections++;
            std::thread worker_t(worker, &active_connections, new_fd, &lock);
            worker_t.detach();
        }
    }
    return 0;
}
