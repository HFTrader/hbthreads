#include "BufferPrinter.h"
#include <gtest/gtest.h>
#include <cstring>

TEST(BufferPrinter, Constructor) {
    BufferPrinter<64> bp;
    EXPECT_EQ(bp.size(), 0);
}

TEST(BufferPrinter, WriteString) {
    BufferPrinter<64> bp;
    bp << "hello";
    EXPECT_EQ(bp.size(), 5);
    EXPECT_EQ(std::string(bp._org, bp.size()), "hello");
}

TEST(BufferPrinter, WriteLiteralArray) {
    BufferPrinter<64> bp;
    bp << "test";
    EXPECT_EQ(bp.size(), 4);
    EXPECT_EQ(std::string(bp._org, bp.size()), "test");
}

TEST(BufferPrinter, WriteMultipleStrings) {
    BufferPrinter<64> bp;
    bp << "hello" << " " << "world";
    EXPECT_EQ(bp.size(), 11);
    EXPECT_EQ(std::string(bp._org, bp.size()), "hello world");
}

TEST(BufferPrinter, WriteUint16) {
    BufferPrinter<64> bp;
    bp << uint16_t(0x1234);
    // Should output "0x1234"
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "0x1234");
}

TEST(BufferPrinter, WriteUint32Small) {
    BufferPrinter<64> bp;
    bp << uint32_t(0x00FF);
    // Small values should be printed as 16-bit
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "0x00ff");
}

TEST(BufferPrinter, WriteUint32Large) {
    BufferPrinter<64> bp;
    bp << uint32_t(0x12345678);
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "0x12345678");
}

TEST(BufferPrinter, WriteUint64Small) {
    BufferPrinter<64> bp;
    bp << uint64_t(0x0000FFFF);
    // The narrowing is transitive: uint64 -> uint32 (fits) -> uint16 (fits),
    // so 0xFFFF prints with 16-bit width, matching this test's own comment and
    // the WriteUint32Small case above. The previous expectation of "0x0000ffff"
    // assumed only a single uint64 -> uint32 step.
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "0xffff");
}

TEST(BufferPrinter, WriteUint64Large) {
    BufferPrinter<128> bp;
    bp << uint64_t(0x123456789ABCDEF0ULL);
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "0x123456789abcdef0");
}

TEST(BufferPrinter, WritePointer) {
    BufferPrinter<64> bp;
    void* ptr = reinterpret_cast<void*>(0x1000);
    bp << ptr;
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "0x1000");
}

TEST(BufferPrinter, WriteMixedTypes) {
    BufferPrinter<128> bp;
    bp << "value=" << uint32_t(42) << " addr=" << reinterpret_cast<void*>(0x1000);
    std::string result(bp._org, bp.size());
    EXPECT_EQ(result, "value=0x002a addr=0x1000");
}

TEST(BufferPrinter, SizeTracking) {
    BufferPrinter<64> bp;
    EXPECT_EQ(bp.size(), 0);
    bp << "12345";
    EXPECT_EQ(bp.size(), 5);
    bp << "67890";
    EXPECT_EQ(bp.size(), 10);
}
