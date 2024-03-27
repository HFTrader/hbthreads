

#include <gtest/gtest.h>
#include "IntrusiveIndexList.h"
#include <vector>

using namespace hbthreads;

// NOLINTBEGIN(clang-diagnostic-sign-compare,readability-function-cognitive-complexity,
// readability-magic-numbers)
TEST(IntrusiveIndexList, Base) {
    struct X {
        int value;
        IntrusiveIndexListHook<uint16_t> hook;
    };
    using XList = IntrusiveIndexList<X, uint16_t, &X::hook, std::vector<X> >;

    XList::Head head;
    const size_t N = 50;
    XList::Container items(N);
    for (size_t j = 0; j < N; ++j) items[j].value = j;
    XList ilist{head, items};
    EXPECT_TRUE(ilist.empty());
    for (size_t j = 0; j < N; ++j) ilist.push_back(j);
    EXPECT_EQ(ilist.front().value, 0);
    EXPECT_EQ(ilist.back().value, N - 1);
    for (size_t j = 0; j < N; ++j) EXPECT_EQ(ilist.at(j).value, j);
    for (size_t j = 0; j < N; ++j) {
        EXPECT_EQ(ilist.front().value, j);
        ilist.pop_front();
    }
    EXPECT_TRUE(ilist.empty());
}
