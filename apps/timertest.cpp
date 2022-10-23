#include "PollReactor.h"
#include "EpollReactor.h"
#include "MallocHooks.h"
#include "Timer.h"

#include <iostream>
#include <array>

using namespace hbthreads;

//--------------------------------------------------
// Example light thread
//--------------------------------------------------

class Worker : public LightThread {
public:
    Worker(std::uint32_t id = 0) : _id(id) {
        printf("Creating worker %d\n", _id);
    }
    ~Worker() {
        printf("Deleting worker %d\n", _id);
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

            printf("Worker %d  fid %d  Event %ld\n", _id, ev->fd, counter++);
        }
    }

private:
    std::uint32_t _id;  //! for printing purposes
};

// Let's just define this here for all the examples
const std::size_t stacksize = 4 * 1024ULL;

void test_poll() {
    Timer timer;
    timer.start(DateTime::msecs(100));
    Pointer<PollReactor> mgr(new PollReactor(storage));
    Pointer<Worker> worker(new Worker);
    mgr->monitor(timer.fd(), worker.get());
    worker->start(stacksize);
    while (mgr->active()) {
        mgr->work();
    }
}

void test_epoll() {
    Timer timer;
    timer.start(DateTime::msecs(100));
    Pointer<EpollReactor> mgr(new EpollReactor(storage, DateTime::msecs(500)));
    Pointer<Worker> worker(new Worker);
    mgr->monitor(timer.fd(), worker.get());
    worker->start(stacksize);
    while (mgr->active()) {
        mgr->work();
    }
}

void test_multi_epoll() {
    std::array<Timer, 5> timers;
    for (Timer& tm : timers) tm.start(DateTime::msecs(100));
    Pointer<EpollReactor> mgr(new EpollReactor(storage));
    std::array<Pointer<Worker>, 15> fleet;
    int counter = 0;
    for (Pointer<Worker>& worker : fleet) {
        worker.reset(new Worker(++counter));
        for (Timer& timer : timers) {
            mgr->monitor(timer.fd(), worker.get());
        }
        worker->start(stacksize);
    }
    while (mgr->active()) {
        mgr->work();
    }
}

int main(int argc, char* argv[]) {
    printf(
        "We are now turning on the malloc hook so every time "
        "malloc(),realloc(),calloc() and free() are called, \n"
        "you will get a printout on the screen with the size "
        "requested (in hex), the result/returned pointer and \n"
        "the caller address. You should see only a couple of "
        "chunky calls to malloc() and respective free().\n"
        "Everything else should be pool-allocated.\n");
    malloc_hook_active = 1;

    //! This pool will allocate-only. It is supposed to be used upstream
    //! by other allocators. In this case we chain it with the one below
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024ULL);

    //! A fast pool that uses bin by size, which makes this super fast
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);

    //! Initialize the TLS for the main thread
    //! As this is __thread specific storage, it cannot be initialized statically
    storage = &buffer;

    printf("--------- Epoll test\n");
    test_epoll();
    printf("--------- Poll test\n");
    test_poll();
    printf("--------- Multi-epoll test\n");
    test_multi_epoll();
}
