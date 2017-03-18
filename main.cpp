#include "server.cpp"

int errno;

int main(int argc, const char * argv[]) {
    struct sockaddr_in server;
    struct sockaddr_in from;
    int socket_descriptor;	
    
    if(starting_server(socket_descriptor, server, from) != 0){
        return errno;
    }
    
    service(socket_descriptor, server, from);
    
    
    return 0;
}
