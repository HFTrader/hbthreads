// Benchmarks for Reactor event dispatch performance

#include <benchmark/benchmark.h>
#include "EpollReactor.h"
#include "PollReactor.h"
#include "Pointer.h"
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace hbthreads;

namespace {

// Thread that waits for events and counts them (never terminates)
class CountingThread : public LightThread {
public:
    int event_count = 0;

    void run() override {
        while (true) {
            Event* ev = wait();
            if (ev && ev->type == EventType::SocketRead) {
                event_count++;
            }
        }
    }
};

// Helper to create an eventfd for testing
int create_eventfd() {
    int fd = eventfd(0, EFD_NONBLOCK);
    if (fd < 0) {
        throw std::runtime_error("Failed to create eventfd");
    }
    return fd;
}

// Helper to trigger an eventfd
void trigger_eventfd(int fd) {
    uint64_t val = 1;
    ::write(fd, &val, sizeof(val));
}

// Helper to consume eventfd
void consume_eventfd(int fd) {
    uint64_t val;
    ::read(fd, &val, sizeof(val));
}

}  // namespace

// Benchmark EpollReactor subscription management
static void BM_EpollReactorSubscribe(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    EpollReactor reactor(&pool, DateTime::msecs(0));
    int fd = create_eventfd();

    Pointer<CountingThread> thread(new CountingThread());
    thread->start(32 * 1024);

    for (auto _ : state) {
        reactor.monitor(fd, thread.get());
        reactor.removeSocket(fd);
    }

    close(fd);
}
BENCHMARK(BM_EpollReactorSubscribe);

// Benchmark PollReactor subscription management
static void BM_PollReactorSubscribe(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    PollReactor reactor(&pool, DateTime::msecs(0));
    int fd = create_eventfd();

    Pointer<CountingThread> thread(new CountingThread());
    thread->start(32 * 1024);

    for (auto _ : state) {
        reactor.monitor(fd, thread.get());
        reactor.removeSocket(fd);
    }

    close(fd);
}
BENCHMARK(BM_PollReactorSubscribe);

// Benchmark EpollReactor event dispatch
static void BM_EpollReactorDispatch(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    const int events_per_iter = 100;

    for (auto _ : state) {
        EpollReactor reactor(&pool, DateTime::msecs(100));
        int fd = create_eventfd();

        Pointer<CountingThread> thread(new CountingThread());
        thread->start(32 * 1024);
        reactor.monitor(fd, thread.get());

        for (int i = 0; i < events_per_iter; i++) {
            trigger_eventfd(fd);
            reactor.work();
            consume_eventfd(fd);
        }

        close(fd);
        benchmark::DoNotOptimize(thread->event_count);
    }

    state.SetItemsProcessed(state.iterations() * events_per_iter);
}
BENCHMARK(BM_EpollReactorDispatch);

// Benchmark PollReactor event dispatch
static void BM_PollReactorDispatch(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    const int events_per_iter = 100;

    for (auto _ : state) {
        PollReactor reactor(&pool, DateTime::msecs(100));
        int fd = create_eventfd();

        Pointer<CountingThread> thread(new CountingThread());
        thread->start(32 * 1024);
        reactor.monitor(fd, thread.get());

        for (int i = 0; i < events_per_iter; i++) {
            trigger_eventfd(fd);
            reactor.work();
            consume_eventfd(fd);
        }

        close(fd);
        benchmark::DoNotOptimize(thread->event_count);
    }

    state.SetItemsProcessed(state.iterations() * events_per_iter);
}
BENCHMARK(BM_PollReactorDispatch);

// Benchmark reactor with multiple file descriptors
static void BM_EpollReactorMultipleFDs(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    const int num_fds = state.range(0);

    for (auto _ : state) {
        EpollReactor reactor(&pool, DateTime::msecs(100));

        std::vector<int> fds;
        std::vector<Pointer<CountingThread>> threads;

        for (int i = 0; i < num_fds; i++) {
            int fd = create_eventfd();
            fds.push_back(fd);

            Pointer<CountingThread> thread(new CountingThread());
            thread->start(32 * 1024);
            threads.push_back(thread);

            reactor.monitor(fd, thread.get());
        }

        // Trigger all fds
        for (int fd : fds) {
            trigger_eventfd(fd);
        }

        // Single work() call to process all
        reactor.work();

        // Cleanup
        for (int fd : fds) {
            close(fd);
        }

        benchmark::DoNotOptimize(threads.size());
    }

    state.SetItemsProcessed(state.iterations() * num_fds);
}
BENCHMARK(BM_EpollReactorMultipleFDs)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

// Benchmark reactor with multiple subscribers per FD
static void BM_EpollReactorMultipleSubscribers(benchmark::State& state) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    storage = &pool;

    const int num_subscribers = state.range(0);

    for (auto _ : state) {
        EpollReactor reactor(&pool, DateTime::msecs(100));
        int fd = create_eventfd();

        std::vector<Pointer<CountingThread>> threads;
        for (int i = 0; i < num_subscribers; i++) {
            Pointer<CountingThread> thread(new CountingThread());
            thread->start(32 * 1024);
            threads.push_back(thread);
            reactor.monitor(fd, thread.get());
        }

        trigger_eventfd(fd);
        reactor.work();

        close(fd);
        benchmark::DoNotOptimize(threads.size());
    }

    state.SetItemsProcessed(state.iterations() * num_subscribers);
}
BENCHMARK(BM_EpollReactorMultipleSubscribers)
    ->Arg(1)
    ->Arg(10)
    ->Arg(100);
