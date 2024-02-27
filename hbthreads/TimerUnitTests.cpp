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
            DateTime finish = start + DateTime::msecs(1000);
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