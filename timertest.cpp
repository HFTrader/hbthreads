#include "PollReactor.h"
#include "EpollReactor.h"
#include "MallocHooks.h"
#include <sys/timerfd.h>
#include <iostream>
#include <array>

using namespace hbthreads;

//--------------------------------------------------
// Example light thread
//--------------------------------------------------

//! Wraps a timerfd which is a file descriptor that will fire events
//! at a predefined interval. This is a kernel facility so you dont
//! have to keep a scheduler in parallel with epoll(). Convenient.
struct Timer {
    int fd;  //! The timerfd file descriptor
    Timer() {
        // Create a non-blocking descriptor
        fd = ::timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);

        // Set to 100 ms interval notification
        itimerspec ts;
        memset(&ts, 0, sizeof(ts));
        ts.it_interval.tv_nsec = 100 * 1000000;
        ts.it_value.tv_nsec = 100 * 1000000;
        int res = ::timerfd_settime(fd, 0, &ts, nullptr);
        if (res != 0) {
            std::cout << "Could not set the alarm:" << std::endl;
            return;
        }
    }
    ~Timer() {
        // Close on destroy
        ::close(fd);
    }
};

class Worker : public LightThread {
public:
    Worker(std::uint32_t id = 0) : _id(id) {
        std::cout << "Creating worker " << _id << std::endl;
    }
    ~Worker() {
        std::cout << "Deleting worker " << _id << std::endl;
    }

    void run() override {
        uint64_t counter = 0;
        for (int j = 0; j < 10; ++j) {
            Event* ev = wait();
            while (true) {
                char buf[8];
                int nb = read(ev->fd, buf, sizeof(buf));
                if (nb < 0) {
                    break;
                }
            }
            std::cout << "Worker " << _id << "  fid " << ev->fd << "  Event " << counter++
                      << std::endl;
        }
    }

private:
    std::uint32_t _id;  //! for printing purposes
};

// Let's just define this here for all the examples
const std::size_t stacksize = 4 * 1024;

void test_poll() {
    Timer timer;
    Pointer<PollReactor> mgr(new PollReactor(storage));
    Pointer<Worker> worker(new Worker);
    mgr->monitor(timer.fd, worker.get());
    worker->start(stacksize);
    while (mgr->active()) {
        mgr->work();
    }
}

void test_epoll() {
    Timer timer;
    Pointer<EpollReactor> mgr(new EpollReactor(storage, DateTime::msecs(500)));
    Pointer<Worker> worker(new Worker);
    mgr->monitor(timer.fd, worker.get());
    worker->start(stacksize);
    while (mgr->active()) {
        mgr->work();
    }
}

void test_multi_epoll() {
    std::array<Timer, 5> timers;
    Pointer<EpollReactor> mgr(new EpollReactor(storage));
    std::array<Pointer<Worker>, 15> fleet;
    int counter = 0;
    for (Pointer<Worker>& worker : fleet) {
        worker.reset(new Worker(++counter));
        for (const Timer& timer : timers) {
            mgr->monitor(timer.fd, worker.get());
        }
        worker->start(stacksize);
    }
    while (mgr->active()) {
        mgr->work();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "We are now turning on the malloc hook so every time "
                 "malloc(),realloc(),calloc() and free() are called, \n"
                 "you will get a printout on the screen with the size "
                 "requested (in hex), the result/returned pointer and \n"
                 "the caller address."
              << std::endl;
    malloc_hook_active = 1;

    //! This pool will allocate-only. It is supposed to be used upstream
    //! by other allocators. In this case we chain it with the one below
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024);

    //! A fast pool that uses bin by size, which makes this super fast
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);

    //! Initialize the TLS for the main thread
    //! As this is __thread specific storage, it cannot be initialized statically
    storage = &buffer;

    std::cout << "--------- Epoll test" << std::endl;
    test_epoll();
    std::cout << "--------- Poll test" << std::endl;
    test_poll();
    std::cout << "--------- Multi-epoll test" << std::endl;
    test_multi_epoll();
}
