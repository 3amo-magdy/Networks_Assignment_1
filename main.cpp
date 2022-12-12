#include <iostream>
#include <pthread.h>
#include <cstdlib>

using namespace std;

int main(){
    std::cout << "yo" << std::endl;
}

void* server_function(){
    
}
/*
Your web server should accept incoming connection requests. 
It should then look for the GET request  and  pick  out  the  name  of  the  requested  file. 
If  the  request  is  POST  then  it  sends  OK message  and  waits  for  the  uploaded  file  from  the  client.  
  a  GET  request  from  a  real WWW client may have several lines of optional information following the GET:
    These optional lines,  though,  will  be  terminated  by  a  blank  line  
    (i.e.,  a  line  containing  zero  or  more  spaces, terminated by a’\r\n’ (carriage return then newline characters).
     Your server should first print out the received command as well as any optional lines following it (preceding the empty line).
     The servershould then respond with the line, this is a very simple version of the real HTTP reply message: 
     HTTP/1.1 200 OK\r\n 
     then in case of GET command only:
     {data, data, ...., data} followed by a blank line "\r\n"
     After finishing the  transmission,  the  server  should  keep  the  connection  open  waiting  for  new  requests  from the same client.
     If the document is not found (in case of GET), the server should respond with (as would a real http server): 
     HTTP/1.1 404 Not Found\r\n
*/