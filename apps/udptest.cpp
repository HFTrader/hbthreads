// UDP client-server pair
// Roughly adapted from
// https://www.geeksforgeeks.org/udp-server-client-implementation-c/

#include "EpollReactor.h"
#include "MallocHooks.h"
#include "Timer.h"
#include "SocketUtils.h"

#include <iostream>
#include <array>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace hbthreads;

//! The server will receive an UDP packet and print its contents
class Server : public LightThread {
public:
    Server() {
    }

    void run() override {
        // Loops, waiting for messages to print
        // If it received "quit", breaks out of the loop
        char buffer[256];
        while (true) {
            // Wait for packet
            Event *ev = wait();

            // It will be filled with the client information
            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));

            // Receive packet
            socklen_t len = sizeof(cliaddr);
            int n = recvfrom(ev->fd, (char *)buffer, sizeof(buffer) - 1, MSG_WAITALL,
                             (struct sockaddr *)&cliaddr, &len);

            // Guarantee that printf() will not overrun the buffer
            buffer[n] = '\0';

            // Print packet
            printf("From(%s:%d): %s\n", inet_ntoa(cliaddr.sin_addr),
                   ntohs(cliaddr.sin_port), buffer);

            // If "quit" was received, finish the loop
            if (::strcmp(buffer, "quit") == 0) break;
        }
        printf("Exiting server loop\n");
    }
};

//! Client will be woken up by an external timer and send a custom
//! packet to the server.
class Client : public LightThread {
public:
    Client(const char *server_address, int server_port) {
        // Filling server information
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(server_port);
        servaddr.sin_addr.s_addr = inet_addr(server_address);
    }

    //! Loop 10 times, waiting first each time for a wake-up event,
    //! that will be provided by a timer (in main)
    void run() override {
        // Create the sender socket
        int fd = createUDPSocket();
        if (fd < 0) return;
        for (int counter = 0; counter < 10; ++counter) {
            // Wait for timer
            Event *ev = wait();
            assert(ev != nullptr);

            // Consume the timer otherwise it will be called again right away
            char buf[8];
            int nb = ::read(ev->fd, buf, sizeof(buf));
            if (nb > 0) {
                // Create UDP packet with a hello message and send
                int sz = ::snprintf(hello, sizeof(hello), "Hello %d", counter);
                sendto(fd, hello, sz, MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                       sizeof(servaddr));
                printf("Client: %s\n", hello);
            }
        }

        // Send a quit command so the server knows it's time to go
        int sz = ::snprintf(hello, sizeof(hello), "quit");
        sendto(fd, hello, sz, MSG_CONFIRM, (const struct sockaddr *)&servaddr,
               sizeof(servaddr));
        printf("Client:%s\n", hello);
        printf("Exiting client loop\n");
    }

    char hello[256];
    struct sockaddr_in servaddr;  //! server address to send messages to
};

// Driver code
int main() {
    // See timertest.cpp for more explanation on these settings
    malloc_hook_active = 1;
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024ULL);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;

    // Perhaps we should get this from the command line - or not
    const char *server_address = "127.0.0.1";
    int server_port = 8080;
    const std::size_t stacksize = size_t(4 * 1024);

    // Create  the light servers for client and server
    Pointer<Client> client(new Client(server_address, server_port));
    client->start(stacksize);
    Pointer<Server> server(new Server());
    server->start(stacksize);

    // Creates and starts the timer
    Timer timer;
    timer.start(DateTime::msecs(100));

    // Typically we would place this socket inside the server object
    // But I wanted to show that it is not strictly necessary
    int server_fd = createAndBindUDPSocket(server_address, server_port);

    // Creates the event loop reactor
    Pointer<EpollReactor> mgr(new EpollReactor(storage, DateTime::msecs(500)));

    // Subscribe the client to the timer
    mgr->monitor(timer.fd, client.get());

    // Subscribe the server to the server socket
    mgr->monitor(server_fd, server.get());

    // Loop until there are not active subscriptions anymore
    while (mgr->active()) {
        mgr->work();
    }

    // Close server socket
    ::close(server_fd);
    return 0;
}