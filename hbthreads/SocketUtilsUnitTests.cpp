#include <gtest/gtest.h>
#include "SocketUtils.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace hbthreads;

TEST(SocketUtils, createTCPSocket) {
    int fd = createTCPSocket();
    EXPECT_GE(fd, 0);

    uint32_t opt;
    socklen_t len;
    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_TYPE, &opt, &len));
    EXPECT_EQ(opt, SOCK_STREAM);

    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &opt, &len));
    EXPECT_EQ(opt, IPPROTO_TCP);

    ::close(fd);
}

TEST(SocketUtils, createAndBindTCPSocket) {
    int fd = createAndBindTCPSocket("127.0.0.1", 19999);
    EXPECT_GE(fd, 0);

    uint32_t opt;
    socklen_t len;
    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_TYPE, &opt, &len));
    EXPECT_EQ(opt, SOCK_STREAM);

    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &opt, &len));
    EXPECT_EQ(opt, IPPROTO_TCP);

    struct sockaddr_in sin;
    len = sizeof(sin);
    EXPECT_EQ(0, getsockname(fd, (struct sockaddr *)&sin, &len));
    EXPECT_EQ(sin.sin_family, AF_INET);
    EXPECT_STREQ((const char *)inet_ntoa(sin.sin_addr), "127.0.0.1");
    EXPECT_EQ(19999, ntohs(sin.sin_port));

    ::close(fd);
}

TEST(SocketUtils, createUDPSocket) {
    int fd = createUDPSocket();
    EXPECT_GE(fd, 0);

    uint32_t opt;
    socklen_t len;
    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_TYPE, &opt, &len));
    EXPECT_EQ(opt, SOCK_DGRAM);

    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &opt, &len));
    EXPECT_EQ(opt, IPPROTO_UDP);

    ::close(fd);
}

TEST(SocketUtils, createAndBindUDPSocket) {
    int fd = createUDPSocket();
    EXPECT_GE(fd, 0);
    EXPECT_TRUE(bindSocket(fd, "127.0.0.1", 19999));

    uint32_t opt;
    socklen_t len;
    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_TYPE, &opt, &len));
    EXPECT_EQ(opt, SOCK_DGRAM);

    len = sizeof(opt);
    EXPECT_EQ(0, getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &opt, &len));
    EXPECT_EQ(opt, IPPROTO_UDP);

    struct sockaddr_in sin;
    len = sizeof(sin);
    EXPECT_EQ(0, getsockname(fd, (struct sockaddr *)&sin, &len));
    EXPECT_EQ(sin.sin_family, AF_INET);
    EXPECT_STREQ((const char *)inet_ntoa(sin.sin_addr), "127.0.0.1");
    EXPECT_EQ(19999, ntohs(sin.sin_port));

    ::close(fd);
}