/* 
 * @file    myHw3.cpp
 *           Adapted from examples in uw1-320-lab.uwb.edu:~css432/examples/, by
 *           Joe McCarthy
 * @brief   This program is meant to send TCP packets in a specific handshake.
 * @author  Brendan Sweeney, SID 1161836
 * @date    November 6, 2012
 */

#include <cstdlib>
#include <stdio.h>
#include <sys/types.h>          // socket, bind
#include <sys/socket.h>         // socket, bind, listen, inet_ntoa
#include <netinet/in.h>         // htonl, htons, inet_ntoa
#include <arpa/inet.h>          // inet_ntoa
#include <netdb.h>              // gethostbyname
#include <unistd.h>             // read, write, close
#include <string.h>             // memset
#include <netinet/tcp.h>        // SO_REUSEADDR
#include <sys/uio.h>            // writev
#include <signal.h>             // sigaction
#include <fcntl.h>              // fcntl
#include <sys/time.h>           // gettimeofday
#include <iostream>

const int BUFSIZE = 1500;

using namespace std;

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc == 2) {
        runServer(argv[1]);
    }
    else if (argc == 3) {
        runClient(argv[1], argv[2]);
    }
    else
        cerr << "Usage: " << argv[0] << " port [hostname]" << endl;
    return 0;
}


void runServer(char* port) {
    int         portNum;                // a server port number
    int         serverSd;               // listen socket
    const int   ARG_COUNT = 3;          // check command line argument count
    const int   ON = 1;                 // for asynchronous connection switch
    const int   ACCEPT = 5;             // number of clients to allow in queue
    sockaddr_in acceptSockAddr;         // address of listen socket
    sockaddr_in newSockAddr;            // address of data socket
    socklen_t   newSockAddrSize = sizeof( newSockAddr );
    struct      sigaction ioaction;     // respond to SIGIO
    
    portNum = atoi(port);
    
    // build address data structure
    memset((char *)&acceptSockAddr, '\0', sizeof(acceptSockAddr));
    acceptSockAddr.sin_family      = AF_INET;   // Address Family Internet
    acceptSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    acceptSockAddr.sin_port        = htons(port);
    memset (&ioaction, '\0', sizeof(ioaction));
    
    // active open, ensure success before continuing
    if ((serverSd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "socket failure" << endl;
        exit(EXIT_FAILURE);
    } // end if ((serverSd = socket(...)))
    
    // setup server socket, bind, and listen for client connection
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&ON, sizeof(int));
    bind(serverSd, (sockaddr *)&acceptSockAddr, sizeof(acceptSockAddr));
    listen(serverSd, ACCEPT);
    
    // sleep indefinitely
    while(true)
    {
        // establish data connection
        serverSd = accept(serverSd, (sockaddr *)&newSockAddr, &newSockAddrSize);
        // SA_SIGINFO field, sigaction() use sa_sigaction field, not sa_handler
        ioaction.sa_flags = SA_SIGINFO;
        if (sigaction(SIGIO, &ioaction, NULL) < 0)
        {
            cerr << "sigaction failure" << endl;
            exit(EXIT_FAILURE);
        } // end if (sigaction(...))
        
        // make asynchronous and sleep
        fcntl(serverSd, F_SETOWN, getpid());
        fcntl(serverSd, F_SETFL, FASYNC);
        sleep(10);
    } // end while(true)

    close(serverSd);
}


void runClient(char* port, char* address) {
    int         portNum;                // a server port number
    int         nreps;                  // repetitions for sending
    int         nbufs;                  // number of data buffers
    int         bufsize;                // size of each data buffer (in bytes)
    char        *serverIp;              // server IP name
    const int   ARG_COUNT = 7;          // verify command line argument count
    int         clientSd;               // for the client-side socket
    int         count = 0;              // read count from server
    struct hostent *host;               // for resolved server from host name
    timeval     start, lap, stop;       // timer checkpoints for output
    sockaddr_in sendSockAddr;           // address data structure
    
    // store arguments in local variables
    portNum  = atoi(port);
    serverIp = address;
    
    // initialize data buffer, clearing any garbage data, and resolve host
    char databuf[nbufs][bufsize];
    memset(databuf, 'B', sizeof(databuf));
    host = gethostbyname(serverIp);
    
    // only continue if host name could be resolved
    if (!host)
    {
        cerr << "unknown hostname: " << serverIp << endl;
        exit(EXIT_FAILURE);
    } // end if (!host)
    
    // build address data structure
    memset((char *)&sendSockAddr, '\0', sizeof(sendSockAddr));
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr =
        inet_addr(inet_ntoa(*(struct in_addr *) *host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    
    // active open, ensure success before continuing
    if ((clientSd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "socket failure" << endl;
        exit(EXIT_FAILURE);
    } // end if ((clientSd = socket(...)))
    
    // only continue if socket connection could be established
    if (connect(clientSd, (sockaddr *)&sendSockAddr, sizeof(sendSockAddr)) < 0)
    {
        cerr << "connect failure" << endl;
        close(clientSd);
        exit(EXIT_FAILURE);
    } // end if (connect(...))
    
    // get read count from server, then close connection
    read(clientSd, &count, sizeof(int));
    close(clientSd);
}
