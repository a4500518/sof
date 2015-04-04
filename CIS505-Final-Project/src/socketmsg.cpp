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

#include "socketmsg.h"

/* valid connection types */
const std::string SocketMsg::SERVER = std::string("SERVER");
const std::string SocketMsg::CLIENT = std::string("CLIENT");

/* valid network protocols */
const int SocketMsg::TCP = SOCK_STREAM;
const int SocketMsg::UDP = SOCK_DGRAM;

/* end of message delimiter, used in TCP */
const char EOM = '~';

/* fixed message size */
const int MSG_BYTES = 500;

/* sets up connection information but does not wait for any communication
 *  Params:
 *      address: if client, the address to speak to. if server, unused
 *      port: the port to initiate a connection on
 *      protocol: ServerMsg::TCP or ServerMsg::UDP
 *      role: ServerMsg::SERVER or ServerMsg::CLIENT
 */
SocketMsg::SocketMsg(std::string address, 
        int port, 
        int protocol,
        std::string role) {    
    /* only client or server connection types are valid */
    assert(
        !role.compare(SocketMsg::SERVER) || 
        !role.compare(SocketMsg::CLIENT)
    );
    
    /* only TCP or UDP socket types are valid */
    assert(
        protocol == SocketMsg::TCP ||
        protocol == SocketMsg::UDP
    );
    
    mListenFd = -1;
    mReadWriteFd = -1;
    mPort = port;
    mProtocol = protocol;
    mRole = role;
    init(address);
}

/* sets up internal state to set up sockets */
void SocketMsg::init(std::string address) {
    struct hostent *hn;
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    
    /* convert hostname to IP if needed */    
    hn = gethostbyname(address.c_str());
    if (hn == NULL) {
        std::cout << "Error: gethostbyname() failed\n" << std::endl;
        std::cout << strerror(errno) << std::endl;
        exit(-1); 
    } else if (hn->h_addr_list[0] == NULL)
        mAddress = address;
    else {
        mAddress = inet_ntoa(*(struct in_addr*)(hn->h_addr_list[0]));
        bcopy(hn->h_addr, &(serv_addr.sin_addr.s_addr), hn->h_length);
    }

    /* common to any connection */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(mPort);

    if (!mRole.compare(SocketMsg::SERVER)) {
        /* set up server connection */
        mListenFd = socket(AF_INET, mProtocol, 0);
        
        /* attempt to open a socket with the given configuration */
        if (mListenFd < 0) {
            std::cout << "Error: socket() failed\n" << std::endl;
            std::cout << strerror(errno) << std::endl;
            exit(-1);
        }
        
        /* bind the socket to the defined port */
        if (bind(mListenFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cout << "Error: bind() failed\n" << std::endl;
            std::cout << strerror(errno) << std::endl;
            exit(-1);        
        }
        
        if (mProtocol == SocketMsg::TCP) {
            /* mark the socket as one which will listen for connections */
            listen(mListenFd, 100);
        } else
            mReadWriteFd = mListenFd;
    } else {
        /* set up client connection */
        mReadWriteFd = socket(AF_INET, mProtocol, 0);

        /* only TCP requires a connect */
        if (mProtocol == SocketMsg::TCP) {        
            /* converts the network address into a binary format for the network */
            if (inet_pton(AF_INET, mAddress.c_str(), &serv_addr.sin_addr) <= 0) {
                std::cout << "Error: inet_pton() failed\n" << std::endl;
                std::cout << strerror(errno) << std::endl;
                exit(-1);
            }

            /* initiate a connection on the socket */
            if (connect(mReadWriteFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                std::cout << "Error: connect() failed\n" << std::endl;
                std::cout << strerror(errno) << std::endl;
                exit(-1);
            }
        }
    }
}

/* send a std::string SocketMsg::response to the last request from 'getMessage()' */
void SocketMsg::sendMessage(std::string message) {
    ssize_t n_bytes = -1;
    ssize_t n_total = 0;

    if (mProtocol == SocketMsg::TCP) 
        message += EOM;

    ssize_t msg_len = message.length();
    const char *buff = message.c_str();

    assert(message.length() <= MSG_BYTES);
        
    if (mProtocol == SocketMsg::TCP) {
        while (n_total != msg_len) {
            n_bytes = write(
                mReadWriteFd, 
                buff + n_total, 
                message.length() - n_total
            );
            if (n_bytes == -1) {
                std::cout << "Error: send() failed\n" << std::endl;
                std::cout << strerror(errno) << std::endl;
                exit(1);
            } else if (n_bytes == 0)
                break;

            n_total += n_bytes;
        }
    } else {
        n_bytes = sendto(
            mReadWriteFd, 
            message.c_str(), 
            message.length(), 
            0,
            (sockaddr *)&serv_addr,
            sizeof(serv_addr)
        );
        if (n_bytes == -1) {
            std::cout << "Error: sendto() failed\n" << std::endl;
            std::cout << strerror(errno) << std::endl;
            exit(-1);
        }
    }
}

/* wait until a message comes in over the network, then return it as a string */
std::string SocketMsg::getMessage() {
    struct sockaddr_in cli_addr;
    socklen_t cli_len;
    char buff[MSG_BYTES] = {};
    int n_total = 0;
    int n_read = -1;

    /* tcp needs additional setup to accept a connection */
    if (!mRole.compare(SocketMsg::SERVER) && mProtocol == SocketMsg::TCP) {
        cli_len = sizeof(cli_addr);
        
        /* if there is an open socket, close it now to avoid leak */
        if (mReadWriteFd >= 0)
            close(mReadWriteFd);
        
        /* establish new connection */
        mReadWriteFd = accept(
            mListenFd, 
            (struct sockaddr *) &cli_addr, 
            &cli_len
        );

        if (mReadWriteFd == -1) {
            std::cout << "Error: accept() failed\n" << std::endl;
            std::cout << strerror(errno) << std::endl;
            exit(-1);
        }
    }
    
    if (mProtocol == SocketMsg::TCP) {
        while (n_total != MSG_BYTES && n_read != 0) {
            n_read = read(mReadWriteFd, buff + n_total, MSG_BYTES - n_total);
        
            if (n_read == -1) {
                std::cout << "Error: read() failed\n" << std::endl;
                std::cout << strerror(errno) << std::endl;
                exit(-1);
            }
            n_total += n_read;
            if (n_total > 0 && buff[n_total - 1] == EOM) {
                buff[n_total - 1] = '\0';
                break;
            }
        }
    } else {
        cli_len = sizeof(serv_addr);
        n_read = recvfrom(mReadWriteFd, buff, MSG_BYTES, 0, (sockaddr *)&serv_addr, &cli_len);
    }
    return std::string(buff);
}

/* closes any connections */
void SocketMsg::shutdown() {
    close(mReadWriteFd);
    close(mListenFd);
}
