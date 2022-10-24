#include "PollReactor.h"
#include "EpollReactor.h"
#include "MallocHooks.h"
#include "Timer.h"
#include "AsmUtils.h"
#include "Histogram.h"

#include <iostream>
#include <array>

using namespace hbthreads;

uint64_t start_ts = tic();

struct FakeReactor : public Reactor {
    using Reactor::Reactor;
    FakeReactor(MemoryStorage* mem) : Reactor(mem) {
    }
    virtual void onSocketOps(int fd, Operation ops){
        // just ignore
    };
    void work() {
        // Every call will generate a notification event
        start_ts = tic();
        notifyEvent(0, EventType::SocketRead);
    }
};

// The worker will just wait for events in an empty loop
struct Worker : public LightThread {
    int64_t numloops;
    Histogram<100> hist;
    Worker(int64_t nloops) : numloops(nloops), hist(0, 500) {
    }
    void run() override {
        for (int64_t j = 0; j < numloops; ++j) {
            wait();
            // Take the time it took to serve the notification
            uint64_t elapsed = tic() - start_ts;

            // Add to stats
            hist.add(elapsed);
        }
    }
};

int main() {
    // Usual to avoid mallocs
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024ULL);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;

    // Create a fake reactor that fires spurious events
    // The goal is that this has zero overhead
    Pointer<FakeReactor> reactor(new FakeReactor(storage));

    // Creates a worker that will accept `numloops` events and quit
    const int64_t numloops = 10000000;
    Pointer<Worker> worker(new Worker(numloops));
    worker->start(4 * 1024);

    // Just monitor some fake FD
    reactor->monitor(0, worker.get());

    // Main loop - take note of start time
    uint64_t t0 = tic();
    while (reactor->active()) {
        reactor->work();
    }

    // Compute total elapsed time and print
    uint64_t elapsed = tic() - t0;
    uint64_t cycles = elapsed / numloops;
    printf("Global:  Average:%ld cycles/iteration or %ld ns on a 3GHz machine\n", cycles,
           cycles / 3);

    // Compute actual reaction time from firing to notification
    // This "should" be about half the global above but we dont take anything for granted
    Stats stats = worker->hist.summary();
    printf("Reaction: Average:%.0f cycles/iteration Median:%.0f cycles/iteration\n",
           stats.average, stats.median);
}
