#include "PollReactor.h"
#include "EpollReactor.h"
#include "MallocHooks.h"
#include "Timer.h"
#include "AsmUtils.h"

#include <iostream>
#include <array>

using namespace hbthreads;

//--------------------------------------------------
// Example light thread
//--------------------------------------------------
struct Stats {
    double median;
    double average;
};

template <size_t N>
struct Histogram {
    struct Bin {
        double sum = 0;
        double sum2 = 0;
        uint32_t count = 0;
    };
    std::array<Bin, N> bins;
    double minimum;
    double maximum;
    Histogram(double min_, double max_) {
        minimum = min_;
        maximum = max_;
    }
    void add(double value) {
        long kbin = lrint(((value - minimum) / (maximum - minimum)) * N);
        if (kbin < 0) kbin = 0;
        if (kbin >= bins.size()) kbin = bins.size() - 1;
        Bin& bin(bins[kbin]);
        bin.sum += value;
        bin.sum2 += value * value;
        bin.count += 1;
    }
    Stats summary() {
        double sum = 0;
        uint64_t count = 0;
        for (const Bin& bin : bins) {
            sum += bin.sum;
            count += bin.count;
        }
        if (count == 0) {
            return {};
        }
        uint64_t totalcount = count;
        double totalsum = sum;
        Stats stats;
        stats.average = totalsum / totalcount;
        sum = 0;
        count = 0;
        for (size_t j = 0; j < bins.size(); ++j) {
            const Bin& bin(bins[j]);
            count += bin.count;
            if (count >= totalcount / 2) {
                stats.median = bin.sum / bin.count;
                break;
            }
        }
        return stats;
    }
};

struct FakeReactor : public Reactor {
    using Reactor::Reactor;
    Histogram<100> hist;
    FakeReactor(MemoryStorage* mem) : Reactor(mem), hist(0, 500) {
    }
    virtual void onSocketOps(int fd, Operation ops){
        // just ignore
    };
    void work() {
        uint64_t start = tic();
        notifyEvent(0, EventType::SocketRead);
        uint64_t elapsed = tic() - start;
        hist.add(elapsed);
    }
};

//--------------------------------------------------
// Example light thread
//--------------------------------------------------

struct Worker : public LightThread {
    int64_t numloops;
    Worker(int64_t nloops) : numloops(nloops) {
    }
    void run() override {
        for (int64_t j = 0; j < numloops; ++j) {
            wait();
        }
    }
};

int main() {
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024ULL);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;

    const int64_t numloops = 10000000;
    Pointer<FakeReactor> reactor(new FakeReactor(storage));
    Pointer<Worker> worker(new Worker(numloops));
    worker->start(4 * 1024);
    reactor->monitor(0, worker.get());

    uint64_t t0 = tic();
    while (reactor->active()) {
        reactor->work();
    }
    uint64_t elapsed = tic() - t0;
    uint64_t cycles = elapsed / numloops;
    printf(
        "Global:  Average:%ld cycles/iteration or %ld ns on a 3GHz "
        "machine\n",
        cycles, cycles / 3);
    Stats stats = reactor->hist.summary();
    printf("Reaction: Average:%.0f cycles/iteration Median:%.0f cycles/iteration\n",
           stats.average, stats.median);
}
