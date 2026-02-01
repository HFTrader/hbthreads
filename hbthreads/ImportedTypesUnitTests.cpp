#include "ImportedTypes.h"
#include <gtest/gtest.h>

using namespace hbthreads;

TEST(ImportedTypes, AllocatorConstruction) {
    // Create a pool resource for testing
    boost::container::pmr::unsynchronized_pool_resource pool;
    Allocator<int> alloc(&pool);

    // Verify the allocator was constructed with the pool
    EXPECT_EQ(alloc.resource(), &pool);
}

TEST(ImportedTypes, FlatSetConstruction) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    FlatSet<int> set(&pool);

    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);
}

TEST(ImportedTypes, FlatSetOperations) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    FlatSet<int> set(&pool);

    set.insert(3);
    set.insert(1);
    set.insert(2);

    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.find(1) != set.end());
    EXPECT_TRUE(set.find(2) != set.end());
    EXPECT_TRUE(set.find(3) != set.end());
    EXPECT_TRUE(set.find(4) == set.end());

    // Verify sorted order (flat_set maintains order)
    auto it = set.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
}

TEST(ImportedTypes, FlatMapConstruction) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    FlatMap<int, std::string> map(&pool);

    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

TEST(ImportedTypes, FlatMapOperations) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    FlatMap<int, std::string> map(&pool);

    map[1] = "one";
    map[2] = "two";
    map[3] = "three";

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[1], "one");
    EXPECT_EQ(map[2], "two");
    EXPECT_EQ(map[3], "three");
}

TEST(ImportedTypes, SmallVectorConstruction) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    SmallVector<int, 8> vec(&pool);

    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);
}

TEST(ImportedTypes, SmallVectorOperations) {
    boost::container::pmr::unsynchronized_pool_resource pool;
    SmallVector<int, 8> vec(&pool);

    // Add elements within small buffer capacity
    for (int i = 0; i < 8; i++) {
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 8);

    // Add more elements (should grow)
    for (int i = 8; i < 16; i++) {
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 16);

    // Verify contents
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(vec[i], i);
    }
}

TEST(ImportedTypes, StackAllocatorExists) {
    // Verify StackAllocator type is available
    // This is boost::context::protected_fixedsize_stack
    StackAllocator stack_alloc(8192);  // 8KB stack

    // Allocate a stack
    auto sctx = stack_alloc.allocate();
    EXPECT_NE(sctx.sp, nullptr);
    EXPECT_GT(sctx.size, 0);

    // Deallocate
    stack_alloc.deallocate(sctx);
}

TEST(ImportedTypes, MemoryStorageType) {
    // Verify MemoryStorage is the correct base type
    boost::container::pmr::unsynchronized_pool_resource pool;
    MemoryStorage* storage = &pool;

    // Should be able to allocate through the base pointer
    void* ptr = storage->allocate(64, 8);
    EXPECT_NE(ptr, nullptr);
    storage->deallocate(ptr, 64, 8);
}
