#include "socketmsg.h"
#include <iostream>
#include <cassert>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
    SocketMsg sm = SocketMsg("localhost", 5000, SocketMsg::UDP, SocketMsg::SERVER);
    std::string msg;
    
    while (true) {
        msg = sm.getMessage();
        std::cout << msg << std::endl;
        std::cout.flush();    
    }
}