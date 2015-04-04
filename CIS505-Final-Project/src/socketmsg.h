#ifndef _SOCKETMSG_H
#define _SOCKETMSG_H

#include <string>
#include <arpa/inet.h>
 
class SocketMsg {
public:
    /* sets up connection information but does not wait for any communication
     *  Params:
     *      address: if client, the address to speak to. if server, unused
     *      port: the port to initiate a connection on
     *      protocol: ServerMsg::TCP or ServerMsg::UDP
     *      role: ServerMsg::SERVER or ServerMsg::CLIENT
     */
    SocketMsg(std::string address, 
        int port, 
        int protocol,
        std::string role);

    /* send a std::string response to the last request from 'getMessage()' */
    void sendMessage(std::string message);
    
    /* wait until a message comes in over the network, then return it as a
     * string 
     */
    std::string getMessage();
    
    /* closes any connections */
    void shutdown();
    
    /* valid connection roles */
    static const std::string SERVER;
    static const std::string CLIENT;
    
    /* valid network protocols */
    static const int TCP;
    static const int UDP;

    
private:
    /* sets up internal state to set up sockets */
    void init(std::string address);
    
    /* the file descriptor to which data can be read or written */
    int mReadWriteFd;
    
    /* the file descriptor from which data can be listened for */
    int mListenFd;
    
    /* if client, the outgoing message port. if server, the listen port */
    int mPort;

    /* ServerMsg::TCP or ServerMsg::UDP */
    int mProtocol;

    /* if client, the address of the server. if server, not used */
    std::string mAddress;

    /* the role SocketMsg::SERVER or SocketMsg::CLIENT */
    std::string mRole;
    
    /* the server address structure */
    struct sockaddr_in serv_addr;
};

#endif
