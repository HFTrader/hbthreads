#include "SocketUtils.h"
// socket includes
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

namespace hbthreads {

int createUDPSocket() {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("createClientSocket: socket() failed");
        return -1;
    }
    return fd;
}

int createAndBindUDPSocket(const char *address, int port) {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("createServerSocket: socket() failed");
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;  // IPv4
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if (bind(fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("createServerSocket: bind() failed");
        return -1;
    }

    return fd;
}

}  // namespace hbthreads
