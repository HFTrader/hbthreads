#include <gtest/gtest.h>
#include "LightThread.h"
#include "Pointer.h"

using namespace hbthreads;

class SimpleThread : public LightThread {
public:
    int run_count = 0;
    int wait_count = 0;

    void run() override {
        run_count++;
        for (int i = 0; i < 5; i++) {
            wait();
            wait_count++;
        }
    }
};

class CountingThread : public LightThread {
public:
    int counter = 0;

    void run() override {
        for (int i = 0; i < 10; i++) {
            counter++;
            wait();
        }
    }
};

class LightThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = new boost::container::pmr::monotonic_buffer_resource(64 * 1024ULL);
        buffer = new boost::container::pmr::unsynchronized_pool_resource(pool);
        storage = buffer;
    }

    void TearDown() override {
        delete buffer;
        delete pool;
        storage = nullptr;
    }

    boost::container::pmr::monotonic_buffer_resource* pool;
    boost::container::pmr::unsynchronized_pool_resource* buffer;
};

TEST_F(LightThreadTest, Constructor) {
    Pointer<SimpleThread> thread(new SimpleThread);
    EXPECT_EQ(thread->run_count, 0);
}

TEST_F(LightThreadTest, StartAndRun) {
    Pointer<SimpleThread> thread(new SimpleThread);
    thread->start(4 * 1024);

    EXPECT_EQ(thread->run_count, 1);
    EXPECT_EQ(thread->wait_count, 0);
}

TEST_F(LightThreadTest, StartAndResume) {
    Pointer<SimpleThread> thread(new SimpleThread);
    thread->start(4 * 1024);

    Event event;
    event.type = EventType::SocketRead;
    event.fd = 1;

    EXPECT_TRUE(thread->resume(&event));
    EXPECT_EQ(thread->wait_count, 1);

    EXPECT_TRUE(thread->resume(&event));
    EXPECT_EQ(thread->wait_count, 2);
}

TEST_F(LightThreadTest, ContextSwitch) {
    Pointer<CountingThread> thread(new CountingThread);
    thread->start(4 * 1024);

    Event event;
    event.type = EventType::SocketRead;
    event.fd = 1;

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(thread->counter, i + 1);
        bool result = thread->resume(&event);
        EXPECT_TRUE(result);
    }

    EXPECT_EQ(thread->counter, 10);
}

TEST_F(LightThreadTest, ThreadCompletion) {
    class ShortThread : public LightThread {
    public:
        void run() override {
            wait();
            // Thread completes after one wait
        }
    };

    Pointer<ShortThread> thread(new ShortThread);
    thread->start(4 * 1024);

    Event event;
    event.type = EventType::SocketRead;
    event.fd = 1;

    // First resume returns true
    EXPECT_TRUE(thread->resume(&event));

    // Second resume returns false (thread completed)
    EXPECT_FALSE(thread->resume(&event));
}

TEST_F(LightThreadTest, MultipleThreads) {
    const int NUM_THREADS = 5;
    Pointer<CountingThread> threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i] = Pointer<CountingThread>(new CountingThread);
        threads[i]->start(4 * 1024);
    }

    Event event;
    event.type = EventType::SocketRead;
    event.fd = 1;

    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < NUM_THREADS; i++) {
            EXPECT_TRUE(threads[i]->resume(&event));
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        EXPECT_EQ(threads[i]->counter, 10);
    }
}

TEST_F(LightThreadTest, EventPassing) {
    class EventCheckThread : public LightThread {
    public:
        EventType received_type;
        int received_fd;

        void run() override {
            Event* ev = wait();
            received_type = ev->type;
            received_fd = ev->fd;
        }
    };

    Pointer<EventCheckThread> thread(new EventCheckThread);
    thread->start(4 * 1024);

    Event event;
    event.type = EventType::SocketError;
    event.fd = 42;

    thread->resume(&event);

    EXPECT_EQ(thread->received_type, EventType::SocketError);
    EXPECT_EQ(thread->received_fd, 42);
}

TEST_F(LightThreadTest, DifferentStackSizes) {
    Pointer<SimpleThread> small_stack(new SimpleThread);
    small_stack->start(4 * 1024);

    Pointer<SimpleThread> large_stack(new SimpleThread);
    large_stack->start(64 * 1024);

    Event event;
    event.type = EventType::SocketRead;
    event.fd = 1;

    // Both should work regardless of stack size
    EXPECT_TRUE(small_stack->resume(&event));
    EXPECT_TRUE(large_stack->resume(&event));
}

TEST_F(LightThreadTest, StackAllocation) {
    class RecursiveThread : public LightThread {
    public:
        int depth = 0;

        void recursive(int n) {
            if (n > 0) {
                depth = n;
                recursive(n - 1);
            }
        }

        void run() override {
            recursive(100);
            wait();
        }
    };

    Pointer<RecursiveThread> thread(new RecursiveThread);
    thread->start(64 * 1024);

    Event event;
    EXPECT_TRUE(thread->resume(&event));
    EXPECT_EQ(thread->depth, 100);
}

TEST_F(LightThreadTest, MultipleWaitResumeCycles) {
    Pointer<SimpleThread> thread(new SimpleThread);
    thread->start(4 * 1024);

    Event event;
    event.type = EventType::SocketRead;
    event.fd = 1;

    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(thread->resume(&event));
        EXPECT_EQ(thread->wait_count, i + 1);
    }

    // Thread should complete after 5 resumes
    EXPECT_FALSE(thread->resume(&event));
}