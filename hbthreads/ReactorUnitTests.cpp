#include <gtest/gtest.h>
#include "EpollReactor.h"
#include "LightThread.h"
#include <sys/eventfd.h>
#include <unistd.h>

using namespace hbthreads;

// Test thread that counts events
class TestThread : public LightThread {
public:
    int events_received = 0;
    EventType last_event_type = EventType::NA;
    int last_fd = -1;

    void run() override {
        while (true) {
            Event* ev = wait();
            events_received++;
            last_event_type = ev->type;
            last_fd = ev->fd;
        }
    }
};

class ReactorTest : public ::testing::Test {
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

TEST_F(ReactorTest, ConstructorDestructor) {
    EpollReactor reactor(buffer);
    EXPECT_FALSE(reactor.active());
}

TEST_F(ReactorTest, MonitorSingleSocket) {
    EpollReactor reactor(buffer);
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());
    EXPECT_TRUE(reactor.active());

    close(fd);
}

TEST_F(ReactorTest, MonitorAndNotify) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());

    // Trigger event
    uint64_t val = 1;
    ASSERT_EQ(write(fd, &val, sizeof(val)), sizeof(val));

    // Process events
    reactor.work();

    EXPECT_EQ(thread->events_received, 1);
    EXPECT_EQ(thread->last_event_type, EventType::SocketRead);
    EXPECT_EQ(thread->last_fd, fd);

    close(fd);
}

TEST_F(ReactorTest, RemoveSocket) {
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

TEST_F(ReactorTest, MultipleThreadsSameSocket) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread1(new TestThread);
    Pointer<TestThread> thread2(new TestThread);
    thread1->start(4 * 1024);
    thread2->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread1.get());
    reactor.monitor(fd, thread2.get());
    EXPECT_TRUE(reactor.active());

    // Trigger event
    uint64_t val = 1;
    ASSERT_EQ(write(fd, &val, sizeof(val)), sizeof(val));

    reactor.work();

    // Both threads should receive the event
    EXPECT_EQ(thread1->events_received, 1);
    EXPECT_EQ(thread2->events_received, 1);

    close(fd);
}

TEST_F(ReactorTest, MultipleSocketsSingleThread) {
    EpollReactor reactor(buffer, DateTime::msecs(10));
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd1 = eventfd(0, EFD_NONBLOCK);
    int fd2 = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);

    reactor.monitor(fd1, thread.get());
    reactor.monitor(fd2, thread.get());
    EXPECT_TRUE(reactor.active());

    // Trigger both events
    uint64_t val = 1;
    ASSERT_EQ(write(fd1, &val, sizeof(val)), sizeof(val));
    ASSERT_EQ(write(fd2, &val, sizeof(val)), sizeof(val));

    reactor.work();

    EXPECT_EQ(thread->events_received, 2);

    close(fd1);
    close(fd2);
}

TEST_F(ReactorTest, RemoveSocketWithMultipleSubscribers) {
    EpollReactor reactor(buffer);
    Pointer<TestThread> thread1(new TestThread);
    Pointer<TestThread> thread2(new TestThread);
    thread1->start(4 * 1024);
    thread2->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread1.get());
    reactor.monitor(fd, thread2.get());
    EXPECT_TRUE(reactor.active());

    // Remove socket should remove all subscriptions
    reactor.removeSocket(fd);
    EXPECT_FALSE(reactor.active());

    close(fd);
}

TEST_F(ReactorTest, DuplicateMonitor) {
    EpollReactor reactor(buffer);
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    // Monitor same FD/thread twice - should not crash
    reactor.monitor(fd, thread.get());
    reactor.monitor(fd, thread.get());
    EXPECT_TRUE(reactor.active());

    close(fd);
}

TEST_F(ReactorTest, Active) {
    EpollReactor reactor(buffer);
    Pointer<TestThread> thread(new TestThread);
    thread->start(4 * 1024);

    EXPECT_FALSE(reactor.active());

    int fd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(fd, 0);

    reactor.monitor(fd, thread.get());
    EXPECT_TRUE(reactor.active());

    reactor.removeSocket(fd);
    EXPECT_FALSE(reactor.active());

    close(fd);
}

TEST_F(ReactorTest, RemoveNonExistentSocket) {
    EpollReactor reactor(buffer);

    // Should not crash when removing non-existent socket
    reactor.removeSocket(999);
    EXPECT_FALSE(reactor.active());
}

TEST_F(ReactorTest, WorkWithNoSockets) {
    EpollReactor reactor(buffer, DateTime::msecs(1));

    // Should return without blocking indefinitely
    EXPECT_TRUE(reactor.work());
    EXPECT_FALSE(reactor.active());
}