#include <gtest/gtest.h>
#include "EpollReactor.h"
#include "LightThread.h"
#include <sys/eventfd.h>
#include <unistd.h>

using namespace hbthreads;

class TestThread : public LightThread {
public:
    int events_received = 0;
    std::vector<EventType> event_types;
    std::vector<int> event_fds;

    void run() override {
        while (true) {
            Event* ev = wait();
            events_received++;
            event_types.push_back(ev->type);
            event_fds.push_back(ev->fd);
        }
    }
};

class EpollReactorTest : public ::testing::Test {
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

TEST_F(EpollReactorTest, Constructor) {
    EpollReactor reactor(buffer);
    EXPECT_FALSE(reactor.active());
}

TEST_F(EpollReactorTest, ConstructorWithTimeout) {
    EpollReactor reactor(buffer, DateTime::msecs(100));
    EXPECT_FALSE(reactor.active());
}

TEST_F(EpollReactorTest, ConstructorWithCustomBufferSize) {
    // Test our recent optimization
    EpollReactor reactor1(buffer, DateTime::msecs(10), 16);
    EpollReactor reactor2(buffer, DateTime::msecs(10), 256);
    EpollReactor reactor3(buffer, DateTime::msecs(10), 1024);

    // Should not crash with different buffer sizes
    EXPECT_FALSE(reactor1.active());
    EXPECT_FALSE(reactor2.active());
    EXPECT_FALSE(reactor3.active());
}

TEST_F(EpollReactorTest, WorkWithoutSubscriptions) {
    EpollReactor reactor(buffer, DateTime::msecs(1));
    EXPECT_TRUE(reactor.work());
}

TEST_F(EpollReactorTest, BasicEventDispatching) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());

    // Trigger event
    uint64_t val = 1;
    ASSERT_EQ(write(fd, &val, sizeof(val)), sizeof(val));

    reactor.work();

    EXPECT_EQ(thread->events_received, 1);
    EXPECT_EQ(thread->event_types[0], EventType::SocketRead);
    EXPECT_EQ(thread->event_fds[0], fd);

    close(fd);
}

TEST_F(EpollReactorTest, MultipleEvents) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd1 = eventfd(0, EFD_NONBLOCK);
    int fd2 = eventfd(0, EFD_NONBLOCK);
    int fd3 = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);
    ASSERT_GE(fd3, 0);

    reactor.monitor(fd1, thread.get());
    reactor.monitor(fd2, thread.get());
    reactor.monitor(fd3, thread.get());

    // Trigger all events
    uint64_t val = 1;
    write(fd1, &val, sizeof(val));
    write(fd2, &val, sizeof(val));
    write(fd3, &val, sizeof(val));

    reactor.work();

    EXPECT_EQ(thread->events_received, 3);

    close(fd1);
    close(fd2);
    close(fd3);
}

TEST_F(EpollReactorTest, LargeEventBuffer) {
    // Test with large event buffer (our optimization)
    EpollReactor reactor(buffer, DateTime::msecs(10), 512);
    Pointer<TestThread> thread(new TestThread);
    thread->start(16 * 1024);

    const int NUM_FDS = 100;
    int fds[NUM_FDS];

    for (int i = 0; i < NUM_FDS; i++) {
        fds[i] = eventfd(0, EFD_NONBLOCK);
        ASSERT_GE(fds[i], 0);
        reactor.monitor(fds[i], thread.get());
    }

    // Trigger all events
    uint64_t val = 1;
    for (int i = 0; i < NUM_FDS; i++) {
        write(fds[i], &val, sizeof(val));
    }

    reactor.work();

    // All events should be processed in single work() call
    EXPECT_EQ(thread->events_received, NUM_FDS);

    for (int i = 0; i < NUM_FDS; i++) {
        close(fds[i]);
    }
}

TEST_F(EpollReactorTest, ErrorEventDispatching) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());

    // Close FD to trigger error/hangup
    close(fd);

    // Trigger an event on the closed FD
    reactor.work();

    // Should have received error or hangup event
    // Note: exact behavior depends on kernel, may get EPOLLERR or EPOLLHUP
}

TEST_F(EpollReactorTest, SocketAddRemove) {
    EpollReactor reactor(buffer);
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());
    EXPECT_TRUE(reactor.active());

    reactor.removeSocket(fd);
    EXPECT_FALSE(reactor.active());

    close(fd);
}

TEST_F(EpollReactorTest, MultipleWorkCalls) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());

    // Trigger event
    uint64_t val = 1;
    write(fd, &val, sizeof(val));

    reactor.work();
    EXPECT_EQ(thread->events_received, 1);

    // Trigger another event
    write(fd, &val, sizeof(val));
    reactor.work();
    EXPECT_EQ(thread->events_received, 2);

    close(fd);
}

TEST_F(EpollReactorTest, NonBlockingTimeout) {
    EpollReactor reactor(buffer, DateTime::msecs(1));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());

    // Don't trigger event, should timeout quickly
    auto start = DateTime::now();
    reactor.work();
    auto end = DateTime::now();

    auto elapsed = (end - start).msecs();
    EXPECT_LE(elapsed, 10);  // Should return within 10ms

    close(fd);
}

TEST_F(EpollReactorTest, MultipleThreadsSameSocket) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread1(new TestThread);
    Pointer<TestThread> thread2(new TestThread);
    thread1->start(4 * 1024);
    thread2->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread1.get());
    reactor.monitor(fd, thread2.get());

    // Trigger event
    uint64_t val = 1;
    write(fd, &val, sizeof(val));

    reactor.work();

    // Both threads should receive the event
    EXPECT_EQ(thread1->events_received, 1);
    EXPECT_EQ(thread2->events_received, 1);

    close(fd);
}

TEST_F(EpollReactorTest, RepeatedAddRemove) {
    EpollReactor reactor(buffer);
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    for (int i = 0; i < 10; i++) {
        reactor.monitor(fd, thread.get());
        EXPECT_TRUE(reactor.active());

        reactor.removeSocket(fd);
        EXPECT_FALSE(reactor.active());
    }

    close(fd);
}