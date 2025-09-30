#include <gtest/gtest.h>
#include "Timer.h"
#include "DateTime.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#include <fcntl.h>

using namespace hbthreads;

TEST(Timer, Constructor) {
    // Expect the fd to be initialized
    Timer timer;
    EXPECT_GE(timer.fd(), 0);

    // Expect the fd to be filled
    int oldfd = timer.fd();
    timer.start(DateTime::secs(1));
    EXPECT_EQ(timer.fd(), oldfd);

    // Expect the magic to be anon
    struct statfs sf;
    EXPECT_EQ(fstatfs(timer.fd(), &sf), 0);
    EXPECT_EQ(sf.f_type, ANON_INODE_FS_MAGIC);

    // Expect the file type to be zero
    struct stat sb;
    EXPECT_EQ(fstat(timer.fd(), &sb), 0);
    EXPECT_EQ(sb.st_mode & S_IFMT, 0);

    // Get the entry corresponding to this file id
    char filename[64];
    int len = ::snprintf(filename, sizeof(filename), "/proc/self/fd/%d", timer.fd());
    EXPECT_GT(len, 14);

    EXPECT_EQ(stat(filename, &sb), 0);
    EXPECT_EQ(sb.st_mode & S_IFMT, 0);

    // Make sure the path has "timerfd" in it
    char path[64];
    len = readlink(filename, path, sizeof(path));  // NOLINT
    EXPECT_GE(len, 14);

    char* res = strstr(path, "timerfd");
    EXPECT_NE(res, nullptr);
    EXPECT_EQ(strncmp(res, "timerfd", 7), 0);
}

TEST(Timer, Start) {
    // Loops creating pairs of (init,interval) and checking for expected behavior
    for (int init = 0; init <= 40; init += 10) {
        DateTime initial = DateTime::msecs(init);
        for (int itv = 10; itv <= 40; itv *= 2) {
            DateTime interval = DateTime::msecs(itv);
            // We want to test the single argument call for init==0
            Timer timer;
            if (init == 0) {
                timer.start(interval);
            } else {
                timer.start(initial, interval);
            }

            // Reads the timer fd and checks the elapsed time for each one
            DateTime start = DateTime::now();
            int fired = (init == 0) ? 1 : 0;
            while (fired < 10) {
                int counter = timer.check();
                if (counter > 0) {
                    ASSERT_EQ(counter, 1);
                    DateTime now = DateTime::now();
                    int64_t elapms = (now - start).msecs();
                    int64_t expected = initial.msecs() + fired * interval.msecs();
                    EXPECT_LE(std::abs(elapms - expected), 2);
                    fired++;
                }
            }
        }
    }
}

TEST(Timer, Stop) {
    Timer timer;
    timer.start(DateTime::msecs(10));

    // Let it fire once
    while (timer.check() == 0) {}

    // Stop the timer
    EXPECT_TRUE(timer.stop());

    // Wait a bit and verify no more events
    usleep(20000);  // 20ms
    EXPECT_EQ(timer.check(), 0);
}

TEST(Timer, OneShot) {
    Timer timer;
    DateTime start_time = DateTime::now();
    DateTime fire_time = start_time + DateTime::msecs(50);

    EXPECT_TRUE(timer.oneShot(fire_time));

    // Wait for the event
    DateTime now = DateTime::now();
    while (now < fire_time + DateTime::msecs(10)) {
        if (timer.check() > 0) {
            break;
        }
        now = DateTime::now();
    }

    // Verify it fired once
    int64_t elapsed = (now - start_time).msecs();
    EXPECT_GE(elapsed, 45);
    EXPECT_LE(elapsed, 60);

    // Wait and verify it doesn't fire again
    usleep(50000);  // 50ms
    EXPECT_EQ(timer.check(), 0);
}

TEST(Timer, Restart) {
    Timer timer;

    // Start with 10ms interval
    timer.start(DateTime::msecs(10));
    while (timer.check() == 0) {}

    // Restart with 20ms interval
    timer.start(DateTime::msecs(20));
    DateTime start = DateTime::now();
    while (timer.check() == 0) {}
    DateTime end = DateTime::now();

    int64_t elapsed = (end - start).msecs();
    EXPECT_GE(elapsed, 18);
    EXPECT_LE(elapsed, 25);
}

TEST(Timer, MultipleChecks) {
    Timer timer;
    timer.start(DateTime::msecs(10));

    // Wait for first event
    while (timer.check() == 0) {}

    // Check again immediately - should be 0
    EXPECT_EQ(timer.check(), 0);

    // Wait for second event
    while (timer.check() == 0) {}
}

TEST(Timer, ZeroInterval) {
    Timer timer;
    // Zero interval should stop the timer
    EXPECT_TRUE(timer.start(DateTime::zero()));

    usleep(10000);  // 10ms
    EXPECT_EQ(timer.check(), 0);
}