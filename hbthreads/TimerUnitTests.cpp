#include <gtest/gtest.h>
#include "Timer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#include <fcntl.h>

using namespace hbthreads;

TEST(Timer, Constructor) {
    // Expect the fd to be initialized as -1
    Timer timer;
    EXPECT_EQ(timer.fd, -1);

    // Expect the fd to be filled
    timer.start(DateTime::secs(1));
    EXPECT_GE(timer.fd, 0);

    // Expect the magic to be anon
    struct statfs sf;
    EXPECT_EQ(fstatfs(timer.fd, &sf), 0);
    EXPECT_EQ(sf.f_type, ANON_INODE_FS_MAGIC);

    // Expect the file type to be zero
    struct stat sb;
    EXPECT_EQ(fstat(timer.fd, &sb), 0);
    EXPECT_EQ(sb.st_mode & S_IFMT, 0);

    // Get the entry corresponding to this file id
    char filename[64];
    int len = ::snprintf(filename, sizeof(filename), "/proc/self/fd/%d", timer.fd);
    EXPECT_GT(len, 14);

    EXPECT_EQ(stat(filename, &sb), 0);
    EXPECT_EQ(sb.st_mode & S_IFMT, 0);

    // Make sure the path has "timerfd" in it
    char path[64];
    len = readlink(filename, path, sizeof(path));
    EXPECT_GE(len, 14);

    char* res = strstr(path, "timerfd");
    EXPECT_NE(res, nullptr);
    EXPECT_EQ(strncmp(res, "timerfd", 7), 0);
}