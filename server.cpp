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

#define PORT 6116 //the port to bind & listen on
#define MAX_CLIENTS 25 // max allowed number of active clients
#define ONE_CLIENT_TIME_OUT 10.0 // the time_out duration if there's only one client connected in seconds

/**
 * computes the duration before time_out based on the number of concurrent connections
 * @param n the number of active connections
 * @return the duration in seconds (ex: 4.324 s)
 */
static float compute_time_out(int n) {
    return ONE_CLIENT_TIME_OUT/n;
}

int worker(int* active_connections, int fd, std::mutex lock){
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
        time_out.tv_usec = (long)((duration- (int)duration)*1000)*1000;
        //set up the parameters for select
        fd_set input;
        FD_ZERO(&input);
        FD_SET(fd, &input);
        int can_read = -1;
        can_read = select(fd + 1, &input, NULL, NULL, &time_out);
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
        //illegal state - shall never happen -:
        if (!FD_ISSET(fd, &input)){
            perror("illegal state!");
            exit(1);
        }
        //there's something to read:

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


























