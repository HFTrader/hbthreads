#include <gtest/gtest.h>
#include "PollReactor.h"
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

class PollReactorTest : public ::testing::Test {
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

TEST_F(PollReactorTest, Constructor) {
    PollReactor reactor(buffer);
    EXPECT_FALSE(reactor.active());
}

TEST_F(PollReactorTest, ConstructorWithTimeout) {
    PollReactor reactor(buffer, DateTime::msecs(100));
    EXPECT_FALSE(reactor.active());
}

TEST_F(PollReactorTest, WorkWithoutSubscriptions) {
    PollReactor reactor(buffer, DateTime::msecs(1));
    reactor.work();
    EXPECT_FALSE(reactor.active());
}

TEST_F(PollReactorTest, BasicEventDispatching) {
    PollReactor reactor(buffer, DateTime::msecs(10));
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

TEST_F(PollReactorTest, DelayedRebuild) {
    PollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd1 = eventfd(0, EFD_NONBLOCK);
    int fd2 = eventfd(0, EFD_NONBLOCK);
    int fd3 = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);
    ASSERT_GE(fd3, 0);

    // Add multiple sockets - should batch rebuild
    reactor.monitor(fd1, thread.get());
    reactor.monitor(fd2, thread.get());
    reactor.monitor(fd3, thread.get());

    // Rebuild happens on first work() call
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

TEST_F(PollReactorTest, MultipleEvents) {
    PollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd1 = eventfd(0, EFD_NONBLOCK);
    int fd2 = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);

    reactor.monitor(fd1, thread.get());
    reactor.monitor(fd2, thread.get());

    // Trigger events
    uint64_t val = 1;
    write(fd1, &val, sizeof(val));
    write(fd2, &val, sizeof(val));

    reactor.work();

    EXPECT_EQ(thread->events_received, 2);

    close(fd1);
    close(fd2);
}

TEST_F(PollReactorTest, ErrorEventDispatching) {
    PollReactor reactor(buffer, DateTime::msecs(10));
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
}

TEST_F(PollReactorTest, SocketAddRemove) {
    PollReactor reactor(buffer);
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

TEST_F(PollReactorTest, MultipleWorkCalls) {
    PollReactor reactor(buffer, DateTime::msecs(10));
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

TEST_F(PollReactorTest, NonBlockingTimeout) {
    PollReactor reactor(buffer, DateTime::msecs(1));
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

TEST_F(PollReactorTest, MultipleThreadsSameSocket) {
    PollReactor reactor(buffer, DateTime::msecs(10));
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

TEST_F(PollReactorTest, RepeatedAddRemove) {
    PollReactor reactor(buffer);
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

TEST_F(PollReactorTest, RebuildAfterRemove) {
    PollReactor reactor(buffer, DateTime::msecs(10));
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

    // Remove middle socket - triggers dirty flag
    reactor.removeSocket(fd2);

    // Trigger remaining sockets
    uint64_t val = 1;
    write(fd1, &val, sizeof(val));
    write(fd3, &val, sizeof(val));

    // Should rebuild and dispatch only fd1 and fd3
    reactor.work();

    EXPECT_EQ(thread->events_received, 2);

    close(fd1);
    close(fd2);
    close(fd3);
}
