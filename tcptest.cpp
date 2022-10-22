// So this is a TCP which is way more complex than a UDP
// one since there are three type of sockets involved:
// 1. client socket
// 2. server acceptance socket
// 3. per-client server socket
// However in contrast with other platforms we will show
// how simple the programming of the downstream side becomes
// with the use of coroutines
// Live Stream: https://youtu.be/ZreWGHvLAmc

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
#include <string.h>

using namespace hbthreads;

//! The server will receive an UDP packet and print its contents
class Server : public LightThread {
    Pointer<Reactor> _reactor;
    std::string _address;
    int _port;
    char buffer[4096];

public:
    Server(Reactor *reactor, const char *address, int port) : _reactor(reactor) {
        _address = address;
        _port = port;
    }
    ~Server() {
    }

    void run() override {
        // Create the socket and bind to the server address/port
        printf("Server::run() create socket\n");
        int server_fd = createAndBindTCPSocket(_address.c_str(), _port);
        if (server_fd < 0) {
            perror("Server::run() createAndBindTCPSocket");
            return;
        }

        // Start tracking this socket
        _reactor->monitor(server_fd, this);

        // Mandatory listening
        printf("Server::run() listen\n");
        int res = ::listen(server_fd, 5);
        if (res < 0) {
            perror("Server::run() listen error");
            return;
        }

        // Loops, waiting for clients to connect
        while (true) {
            // Two types of events can come through here:
            // 1. Accept requests through the socket server
            // 2. Data through accepted sockets
            Event *ev = wait();

            // First case, this is about handling connect requests
            if (ev->fd == server_fd) {
                struct sockaddr_in clientaddr;
                memset(&clientaddr, 0, sizeof(clientaddr));

                socklen_t len = sizeof(clientaddr);
                int client_fd = ::accept(server_fd, (sockaddr *)&clientaddr, &len);
                if (client_fd < 0) {
                    perror("Server::run() accept");
                    continue;
                }

                // We could do two things at this point:
                // 1. Create a new object and monitor this socket into it
                // 2. Just be lazy and funnel all data to this object (actual choice)
                _reactor->monitor(client_fd, this);

            } else {
                // This is actual data through connections
                if (ev->type == EventType::SocketRead) {
                    printf("Server::run() Client socket read\n");
                    // It will be filled with the client information
                    struct sockaddr_in cliaddr;
                    memset(&cliaddr, 0, sizeof(cliaddr));

                    // Receive packet
                    socklen_t len = sizeof(cliaddr);
                    int n = recvfrom(ev->fd, (char *)buffer, sizeof(buffer) - 1,
                                     MSG_DONTWAIT, (struct sockaddr *)&cliaddr, &len);

                    // Guarantee that printf() will not overrun the buffer
                    buffer[n] = '\0';

                    // Print packet
                    printf("From(%s:%d): %s\n", inet_ntoa(cliaddr.sin_addr),
                           (int)ntohs(cliaddr.sin_port), buffer);

                    // If "quit" was received, finish the loop
                    if (::strchr(buffer, 0xFF) != NULL) break;

                } else if ((ev->type == EventType::SocketError) ||
                           (ev->type == EventType::SocketHangup)) {
                    // Something bad happened
                    _reactor->removeSocket(ev->fd);
                }
            }
        }

        // be a good citizen
        ::close(server_fd);
        printf("Exiting server loop\n");
    }
};

//! Client will be woken up by an external timer and send a custom
//! packet to the server.
class Client : public LightThread {
    Pointer<Reactor> _reactor;
    char hello[256];
    struct sockaddr_in servaddr;  //! server address to send messages to

public:
    Client(Reactor *reactor, const char *server_address, int server_port)
        : _reactor(reactor) {
        // Filling server information
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(server_port);
        servaddr.sin_addr.s_addr = inet_addr(server_address);
    }

    bool connect(int fd) {
        while (true) {
            // Attempt to connect
            // On a 2nd pass this will check that the socket is connected already
            int res = ::connect(fd, (sockaddr *)&servaddr, sizeof(servaddr));
            if (res == 0) {
                printf("Socket connected\n");
                break;
            }

            // As the socket is non-blocking, it will return EINPROGRESS
            // until it connects finally
            if (errno != EINPROGRESS) {
                perror("Client::run() connect");
                ::close(fd);
                return false;
            }

            // this is released by the timer event
            Event *ev = wait();
            if (ev->fd == fd) {
                if ((ev->type != EventType::SocketRead) ||
                    (ev->type != EventType::SocketHangup)) {
                    printf("Client::run(): socket error \n");
                    return false;
                }
            }
        }
        return true;
    }

    //! Loop 10 times, waiting first each time for a wake-up event,
    //! that will be provided by a timer (in main)
    void run() override {
        // Create the sender socket
        printf("Client::run() create socket\n");
        int fd = createTCPSocket();
        if (fd < 0) return;

        // We need to set nonblocking so connect will not block
        printf("Client::run() set nonblocking\n");
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        // Monitor the socket
        _reactor->monitor(fd, this);

        // Creates and starts the timer
        // This timer will both serve to help check the connect() but also
        // "wake up" the thread to send data
        Timer timer;
        timer.start(DateTime::msecs(100));

        // Subscribe the client to the timer
        _reactor->monitor(timer.fd, this);

        // Connect to server
        if (!connect(fd)) return;

        // Here we are connected
        for (int counter = 0; counter < 10; ++counter) {
            // Wait for timer
            Event *ev = wait();
            assert(ev != nullptr);

            // Consume the timer otherwise it will be called again right away
            char buf[8];
            int nb = ::read(ev->fd, buf, sizeof(buf));

            // Create TCP message with a hello message and send
            int sz = ::snprintf(hello, sizeof(hello), "Hello %d", counter);
            int res = ::send(fd, hello, sz, MSG_DONTWAIT);
            if (res <= 0) {
                perror("Client::run() send");
                break;
            }
            printf("Client: %s\n", hello);
        }

        // Send a quit command so the server knows it's time to go
        hello[0] = 0xFF;
        hello[1] = 0;
        int res = send(fd, hello, 1, MSG_WAITALL);
        if (res <= 0) {
            perror("Client::run() send");
        }
        printf("Client:%s\n", hello);
        printf("Exiting client loop\n");
    }
};

// Driver code
int main() {
    // See timertest.cpp for more explanation on these settings
    malloc_hook_active = 1;
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;

    // Perhaps we should get this from the command line - or not
    const char *server_address = "127.0.0.1";
    int server_port = 8080;
    const std::size_t stacksize = 4 * 1024;

    // Creates the event loop reactor
    Pointer<EpollReactor> mgr(new EpollReactor(storage, DateTime::nsecs(-1)));

    // Create  the light servers for client and server
    Pointer<Server> server(new Server(mgr.get(), server_address, server_port));
    server->start(stacksize);

    Pointer<Client> client(new Client(mgr.get(), server_address, server_port));
    client->start(stacksize);

    // Loop until there are not active subscriptions anymore
    while (mgr->active()) {
        mgr->work();
    }

    return 0;
}