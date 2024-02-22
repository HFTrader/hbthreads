#include "EpollReactor.h"
#include "MallocHooks.h"
#include "Timer.h"
#include "SocketUtils.h"
#include "StringUtils.h"

#include <iostream>
#include <array>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace hbthreads;

struct MCastListener : public LightThread {
    //! Constructor
    MCastListener() {
    }
    void run() override {
        // Create the sender socket
        char buf[4096];
        while (true) {
            // Wait for timer
            Event* ev = wait();
            if (ev == nullptr) continue;

            // Consume the timer otherwise it will be called again right away
            ssize_t nb = ::read(ev->fd, buf, sizeof(buf));
            if (nb < 0) break;
            if (nb == 0) continue;
            printhex(std::cout, buf, nb, "0x", 32);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: mclisten <address> <port> [<interface>]\n");
        return 0;
    }
    const char* mcaddress = argv[1];
    int mcport = ::atoi(argv[2]);
    const char* interface = argv[3];

    // See timertest.cpp for more explanation on these settings
    malloc_hook_active = 1;
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024 * 1024ULL);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;

    const std::size_t stacksize = size_t(32 * 1024);

    int sockfd = createUDPSocket();
    if (sockfd < 0) {
        return 2;
    }
    if (!setSocketMulticastJoin(sockfd, mcaddress, mcport, interface)) {
        close(sockfd);
        sockfd = -1;
        return 1;
    }

    // Create  the light servers for MCastListener and server
    Pointer<MCastListener> mc(new MCastListener);
    mc->start(stacksize);

    // Creates the event loop reactor
    Pointer<EpollReactor> mgr(new EpollReactor(storage, DateTime::msecs(500)));

    // Subscribe the server to the server socket
    mgr->monitor(sockfd, mc.get());

    // Loop until there are not active subscriptions anymore
    while (mgr->active()) {
        mgr->work();
    }

    // Close server socket
    ::close(sockfd);
    return 0;
}
