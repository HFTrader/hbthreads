#include "SocketUtils.h"
// socket includes
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>

namespace hbthreads {

int createTCPSocket() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("createClientSocket: socket() failed");
        return -1;
    }
    return fd;
}

int createAndBindTCPSocket(const char *address, int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
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

    int flags = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) < 0)
        perror("createAndBindTCPSocket(): setsockopt(SO_REUSEADDR) failed");

    // Bind the socket with the server address
    if (bind(fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("createAndBindTCPSocket(): bind() failed");
        return -1;
    }

    return fd;
}

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

bool setSocketNonBlocking(int sockid) {
    // non-blocking socket
    int flags = 0;
    int res = ::fcntl(sockid, F_GETFL, &flags, sizeof(flags));
    flags |= O_NONBLOCK;
    res = ::fcntl(sockid, F_SETFL, flags);
    if (res < 0) {
        perror("Multicast fcntl(O_NONBLOCK):");
        return false;
    }
    return true;
}

bool parseIPAddress(const char *address, int server_port, sockaddr_in *addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = htons(server_port);
    return parseIPAddress(address, &addr->sin_addr);
}

bool parseIPAddress(const char *address, in_addr *addr) {
    if (address == nullptr) {
        addr->s_addr = INADDR_ANY;
        return true;
    }

    // Try as an IP address
    int res = inet_aton(address, addr);
    if (res == 1) return true;

    // Try as an interface name
    int sockid = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);  // Create a fake socket
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

#pragma GCC diagnostic push
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
    ::strncpy(ifr.ifr_name, address, sizeof(ifr.ifr_name));
#pragma GCC diagnostic pop

    res = ioctl(sockid, SIOCGIFADDR, (caddr_t)&ifr, sizeof(struct ifreq));
    if (res != 0) {
        int err = errno;
        fprintf(stderr, "ioctl(SIOCGIFADDR): %s\n", strerror(err));
    }
    ::close(sockid);
    if (res != 0) {
        return false;
    }
    *addr = ((sockaddr_in *)&(ifr.ifr_addr))->sin_addr;
    return true;
}

bool setSocketMulticastJoin(int sockid, const char *ipaddr, int port,
                            const char *interface) {
    // Create address
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(struct sockaddr_in));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(port);
    if (!parseIPAddress(ipaddr, &bind_addr.sin_addr)) {
        fprintf(stderr, "Could not translate address %s\n", ipaddr);
        return false;
    }

    // create request
    struct ip_mreq groupreq;
    memset(&groupreq, 0, sizeof(groupreq));
    groupreq.imr_multiaddr = bind_addr.sin_addr;
    if (!parseIPAddress(interface, &groupreq.imr_interface)) {
        fprintf(stderr, "Could not translate address %s\n", interface);
        return false;
    }

    // join
    int res = setsockopt(sockid, SOL_IP, IP_ADD_MEMBERSHIP, (void *)&groupreq,
                         sizeof(groupreq));
    if (res < 0) {
        fprintf(stderr, "setSocketMulticastJoin()::setsockopt(IP_ADD_MEMBERSHIP) %s\n",
                strerror(errno));
        return false;
    }
    return true;
}

}  // namespace hbthreads
