#include "PollReactor.h"
#include "EpollReactor.h"
#include "MallocHooks.h"
#include "Timer.h"
#include "AsmUtils.h"
#include "Histogram.h"
#include "Timer.h"

#include <sys/eventfd.h>

using namespace hbthreads;

uint64_t start_tic = 0;

/**
 * The producer thread waits for events and fires a notification
 */
struct Producer : public LightThread {
    Producer(int fd, int64_t nloops) : efd(fd), numloops(nloops) {
    }
    int efd;
    int64_t numloops;
    void run() override {
        for (int64_t j = 0; j < numloops; ++j) {
            // Wait for event
            Event* ev = wait();

            // Consume the timer
            char buf[32];
            size_t nb = ::read(ev->fd, buf, sizeof(buf));

            // Fire the notification
            eventfd_write(efd, 1);

            // Take note of the current time
            start_tic = tic();
        }
    }
};

/**
 * Consumer thread waits for notifications and adds the elapsed time
 * to histogram
 */
struct Consumer : public LightThread {
    int64_t numloops;
    Histogram<500> hist;
    Consumer(int64_t nloops) : numloops(nloops), hist(0, 500) {
    }
    void run() override {
        for (int64_t j = 0; j < numloops; ++j) {
            // Wait for event
            Event* ev = wait();

            // Calculate elapsed time since firing the event
            uint64_t elapsed = tic() - start_tic;

            // Add to histogram
            hist.add(elapsed);

            // Reset the eventfd counter
            eventfd_t value;
            eventfd_read(ev->fd, &value);
            eventfd_write(ev->fd, -1);
        }
    }
};

int main() {
    // Usual memory pooling
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024ULL);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;

    // Create epoll reactor as non-blocking
    const int64_t numloops = 100;
    Pointer<EpollReactor> reactor(new EpollReactor(storage, DateTime::msecs(0)));

    // Create the eventfd descriptor
    int efd = eventfd(0, 0);

    // Create producer (eventfd writer)
    Pointer<Producer> producer(new Producer(efd, numloops));
    producer->start(4 * 1024);

    // Create consumer (eventfd notified)
    Pointer<Consumer> worker(new Consumer(numloops));
    worker->start(4 * 1024);

    // Fires the producer from a timer
    Timer timer;
    timer.start(DateTime::msecs(50));
    reactor->monitor(timer.fd(), producer.get());

    // Link eventfd descriptor to consumer
    reactor->monitor(efd, worker.get());

    // Loop while the two threads are active
    while (reactor->active()) {
        reactor->work();
    }

    // Print stats
    Stats stats = worker->hist.summary();
    printf("Reaction: Average:%.0f cycles/iteration Median:%.0f cycles/iteration\n",
           stats.average, stats.median);
}
