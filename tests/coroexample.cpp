#include "LightThread.h"
#include <stdlib.h>

using namespace hbthreads;

void dosomething() {
    printf("Doing something else\n");
}

int buy() {
    return rand() % 10;
}

int sell(int amount) {
    if (amount == 0) return 0;
    return rand() % (amount + 1);
}

static int MAXLOOPS = 5;
static int MAXCOUNT = 3;

//////////////////////////////////////////////////
// Coroutines part
//////////////////////////////////////////////////

/**
 * A coroutines version of the Trader algorithm, below
 */
struct Worker : public LightThread {
    void run() override {
        int position = 0;
        for (int loop = 0; loop < MAXLOOPS; ++loop) {
            // Acquire position
            for (int counter = 0; counter < MAXCOUNT; ++counter) {
                wait();
                position += buy();
                printf("Buying Loop:%d Counter:%d Position:%d\n", loop, counter,
                       position);
            }
            // Unwind position
            while (position > 0) {
                wait();
                position -= sell(position);
                printf("Selling Position:%d\n", position);
            }
        }
    }
};

void coro_example() {
    Pointer<Worker> worker(new Worker);
    worker->start(4 * 1024);
    Event fake;
    printf("------------ Coroutine\n");
    printf("Start trading\n");
    while (worker->resume(&fake)) {
    }
    printf("Finish trading\n");
}

//////////////////////////////////////////////////
// Traditional programming
//////////////////////////////////////////////////

/**
 * A Traditional trader algorithm
 */
struct Trader {
    int position;
    int loop;
    int counter;
    enum State { Buying = 1, Selling = 2 };
    State state;
    Trader() {
        position = 0;
        loop = 0;
        counter = 0;
        state = Buying;
    }
    bool work() {
        if (state == Buying) {
            position += buy();
            counter += 1;
            if (counter >= MAXCOUNT) {
                counter = 0;
                loop += 1;
                state = Selling;
            }
            printf("Buying Loop:%d Counter:%d Position:%d\n", loop, counter, position);
        } else if (state == Selling) {
            position -= sell(position);
            if (position == 0) {
                state = Buying;
                counter = 0;
            }
            printf("Selling Position:%d\n", position);
        }
        bool finished = ((state == Buying) && (loop >= MAXLOOPS));
        return !finished;
    }
};

void traditional_example() {
    Trader trader;
    printf("------------ Traditional\n");
    printf("Start trading\n");
    while (trader.work()) {
    }
    printf("Finish trading\n");
}

int main() {
    boost::container::pmr::monotonic_buffer_resource pool(8 * 1024ULL);
    boost::container::pmr::unsynchronized_pool_resource buffer(&pool);
    storage = &buffer;
    traditional_example();
    coro_example();
}