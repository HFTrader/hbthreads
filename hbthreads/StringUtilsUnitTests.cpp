#include "StringUtils.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace hbthreads;

TEST(StringUtils, printhex) {
    char data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    {
        std::ostringstream out;
        printhex(out, data, sizeof(data), "prefix", 8);
        EXPECT_EQ(out.str(), "prefix0000  00 01 02 03 04 05 06 07  ........        \n");
    }
    {
        std::ostringstream out;
        printhex(out, data, sizeof(data), "prefix", 7);
        EXPECT_EQ(out.str(),
                  "prefix0000  00 01 02 03 04 05 06  .......       \n"
                  "prefix0007  07                    .             \n");
    }
}