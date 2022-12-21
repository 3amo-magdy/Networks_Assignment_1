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
#include "common.h"
#define PORT 6116 //the port to bind & listen on
#define MAX_CLIENTS 25 // max allowed number of active clients
#define ONE_CLIENT_TIME_OUT 10.0 // the time_out duration if there's only one client connected in seconds
#define CHUNK_SIZE 1024
static const std::string error_code = "HTTP/1.1 500 Internal Server Error\r\n";
static const std::string success_code = "HTTP/1.1 200 OK\r\n";
static const std::string clength_header = "content-length: ";

/**
 * computes the duration before time_out based on the number of concurrent connections
 * @param n the number of active connections
 * @return the duration in seconds (ex: 4.324 s)
 */
static float compute_time_out(int n) {
    return ONE_CLIENT_TIME_OUT/n;
}

int min(int a, int b) {
    if(a<=b)
        return a;
    return b;
}

int worker(int* active_connections, int fd, std::mutex lock){
    /**
     * modes:
     * 0: waiting for request
     * 1: downloading a chunk data onto a file
     */
    int mode = 0;
    int size_to_read = 0;
    std::string command;
    std::string file_path;

    while(true){
        //critical section - on resource: active_connections
        //compute the duration based on the heuristic
        float duration;
        lock.lock();
        duration = compute_time_out(*active_connections);
        lock.unlock();
        //build the timeval struct to call select
        timeval time_out{};
        time_out.tv_sec = (int)duration;
        time_out.tv_usec = (long)((duration - (int)duration)*1000)*1000;
        //set up the parameters for select
        fd_set input;
        FD_ZERO(&input);
        FD_SET(fd, &input);
        int can_read = -1;
        //wait till time_out or something is there to read
        can_read = select(fd + 1, &input, nullptr, nullptr, &time_out);
        if (can_read == -1) {
            perror("select err in worker");
            exit(1);
        }
        //if time_out occurred:
        else if (can_read == 0){
            lock.lock();
            *active_connections = *active_connections - 1;
            lock.unlock();
            close(fd);
            return 0;
        }
        //illegal state -must never happen -:
        if (!FD_ISSET(fd, &input)){
            perror("illegal state!");
            exit(1);
        }
        //there's something to read:
        //read the chunk (unblocking after select()):
        char* buffer= (char*) malloc(CHUNK_SIZE);
        int buffer_offset = 0;
        int buffer_size = read(fd,buffer,CHUNK_SIZE);
        if(buffer_size==0){
            //close connection
            lock.lock();
            *active_connections = *active_connections - 1;
            lock.unlock();
            close(fd);
            return 0;
        }
        if(buffer_size==-1){
            perror("buffer reading err | worker");
            exit(1);
        }
        //if in mode [waiting for a request]
        if(mode==0) {
            //parse the chunk into -> request, file_path, content_length(in case of post)
            std::string request = "";
            for (int i = 0;; i++) {
                char next_char = buffer[buffer_offset++];
                if (next_char == '\r') {
                    buffer_offset++;//skip '\n
                    break;
                }
                request += next_char;
            }
            std::vector<std::string> request_words = get_words(request);
            command = request_words.at(0);
            file_path = request_words.at(1);
            if (command == "GET") {
                std::ifstream in(file_path,std::ios::binary);
                if(in.fail()){
                    //send error
                    write(fd,error_code.c_str(), error_code.size());
                    perror("failed to open the file | executing POST");
                }
                //allocate the chunk to be sent in memory
                char* chunk = (char*)(malloc(CHUNK_SIZE));
                strncpy(chunk, (success_code.c_str()),success_code.size());
                //int to track the current chunk size
                int filled = success_code.size();

                //get file size
                in.seekg(std::ios::end);
                int file_size = in.tellg();
                in.seekg(std::ios::beg);

                //write content-length header:
                strncpy(chunk,clength_header.c_str(),clength_header.size());
                filled+=(int)clength_header.size();
                for(int i=3;i>=0;i--){
                    chunk[filled++]=(char)((file_size >> 3*i)&0xFF);
                }
                //write "\r\n" at end of header section
                chunk[filled++]='\r';
                chunk[filled++]='\n';
                //write file data onto chunks and keep sending chunks till the whole file is sent
                int file_bytes_copied = 0;
                while (file_bytes_copied < file_size && !in.eof()){
                    if(filled==0){
                        memset(chunk,'\0',CHUNK_SIZE);
                    }
                    int file_bytes_to_copy = min(CHUNK_SIZE-filled, file_size - file_bytes_copied);
                    in.read(chunk, file_bytes_to_copy);
                    file_bytes_copied+=file_bytes_to_copy;
                    write(fd,chunk,file_bytes_to_copy+filled);
                    filled = 0;
                }
            }
            else if (command == "POST") {
                std::string clength_header_value;
                for (int i = 0;; i++) {
                    char next_char = buffer[buffer_offset++];
                    if (next_char == '\r') {
                        buffer_offset++;//skip '\n'
                        break;
                    }
                    clength_header_value += next_char;
                }
                buffer_offset+=2;//skip end of header section '\r\n'
                std::vector<std::string> content_length_words = get_words(request);
                if (content_length_words.size() != 2 || content_length_words.at(0) != "content-length:") {
                    write(fd,error_code.c_str(), error_code.size());
                    perror("error parsing POST");
                }
                size_to_read = stoi(content_length_words.at(1));
                //file writing:
                std::ofstream out(file_path, std::ios::binary);
                if (out.fail()) {
                    write(fd,error_code.c_str(), error_code.size());
                    perror("failed to open the file | executing POST");
                }
                out.write(buffer, buffer_size - buffer_offset);
                if (out.bad()) {
                    write(fd,error_code.c_str(), error_code.size());
                    out.close();
                    perror("error writing | executing POST");
                }
                out.close();
                mode = 1;
            } else {
                write(fd,error_code.c_str(), error_code.size());
                perror("unsupported command");
            }
        }
        else if(mode==1){
            bzero(buffer,CHUNK_SIZE);
            int chunk_bytes_read = read(fd,buffer,sizeof(buffer));
            if(chunk_bytes_read==0){
                //close connection
                lock.lock();
                *active_connections = *active_connections - 1;
                lock.unlock();
                close(fd);
                return 0;
            }
            if(chunk_bytes_read==-1){
                perror("buffer reading err | worker mode = 1");
                exit(1);
            }
            size_to_read-=chunk_bytes_read;
            if(size_to_read<0){
                perror("wrong content-length received from client !");
            }
            if(size_to_read==0){
                mode = 0;
            }
            //write append onto file
            std::ofstream out(file_path, std::ios::binary);
            if (out.fail()) {
                perror("failed to open the file | executing POST mode 1 !!!");
            }
            out.write(buffer, chunk_bytes_read);
            if (out.bad()) {
                out.close();
                perror("failed to write to the file | executing POST mode 1 !!!");
            }
            out.close();

        }
    }
}

int main(){
    int active_connections=0;
    std::mutex lock;
    int listener; // listening socket descriptor

    int yes=1; // for setsockopt() SO_REUSEADDR, below
    int addrlen;

    //create the server socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket err");
        exit(1);
    }
    //allow the program to reuse the port
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
        perror("setsockopt err");
        exit(1);
    }
    sockaddr_in server_addr_in{}; //server address
    sockaddr_in client_addr_in{}; //client address

    // build the addr struct
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_addr.s_addr = INADDR_ANY;
    server_addr_in.sin_port = htons(PORT);
    server_addr_in.sin_zero =
    wmemset(reinterpret_cast<wchar_t *>(&(server_addr_in.sin_zero)), 0, sizeof(server_addr_in.sin_zero));
    //bind the socket to the ip & port
    if (bind(listener, (struct sockaddr*) &server_addr_in, sizeof(server_addr_in)) == -1) {
        perror("bind err");
        exit(1);
    }
    //make the socket start the socket listening to client connections
    if (listen(listener, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(1);
    }
    //the server loop
    while(true){
        // handle new connections
        addrlen = sizeof(client_addr_in);
        int new_fd = -1;
        new_fd = accept(listener, (struct sockaddr*) &client_addr_in, (socklen_t*)(&addrlen))
        if (new_fd == -1) {
            perror("accept err");
        } else {
            printf("new connection from %s on fd %d\n", inet_ntoa(client_addr_in.sin_addr), new_fd);
            std::thread worker = new std::thread()
        }
    }
}


























