#include "MallocHooks.h"
#include <gtest/gtest.h>
#include <cstdlib>

// Note: These tests verify the malloc hook infrastructure exists and can be toggled.
// The actual hooking behavior (printing to stderr) is tested indirectly since
// capturing stderr output in unit tests is complex and platform-dependent.

TEST(MallocHooks, HookFlagExists) {
    // Verify the global flag exists and is accessible
    EXPECT_EQ(malloc_hook_active, 0);
}

TEST(MallocHooks, HookFlagToggle) {
    // Save original state
    int original = malloc_hook_active;

    // Toggle on
    malloc_hook_active = 1;
    EXPECT_EQ(malloc_hook_active, 1);

    // Toggle off
    malloc_hook_active = 0;
    EXPECT_EQ(malloc_hook_active, 0);

    // Restore original state
    malloc_hook_active = original;
}

TEST(MallocHooks, MallocWorksWithHookDisabled) {
    malloc_hook_active = 0;

    // Basic allocation should work
    void* ptr = malloc(64);
    EXPECT_NE(ptr, nullptr);

    // Free should work
    free(ptr);
}

TEST(MallocHooks, CallocWorksWithHookDisabled) {
    malloc_hook_active = 0;

    // Calloc should work and zero-initialize
    int* ptr = static_cast<int*>(calloc(10, sizeof(int)));
    EXPECT_NE(ptr, nullptr);

    // Verify zero-initialization
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(ptr[i], 0);
    }

    free(ptr);
}

TEST(MallocHooks, ReallocWorksWithHookDisabled) {
    malloc_hook_active = 0;

    // Initial allocation
    void* ptr = malloc(32);
    EXPECT_NE(ptr, nullptr);

    // Realloc to larger size
    void* ptr2 = realloc(ptr, 128);
    EXPECT_NE(ptr2, nullptr);

    // Realloc to smaller size
    void* ptr3 = realloc(ptr2, 16);
    EXPECT_NE(ptr3, nullptr);

    free(ptr3);
}

TEST(MallocHooks, MallocWorksWithHookEnabled) {
    // Enable hooks (will print to stderr, but should still work)
    malloc_hook_active = 1;

    void* ptr = malloc(64);
    EXPECT_NE(ptr, nullptr);

    free(ptr);

    // Disable hooks
    malloc_hook_active = 0;
}

TEST(MallocHooks, MultipleAllocationsWithHookEnabled) {
    malloc_hook_active = 1;

    // Multiple allocations
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(64 + i * 16);
        EXPECT_NE(ptrs[i], nullptr);
    }

    // Free all
    for (int i = 0; i < 10; i++) {
        free(ptrs[i]);
    }

    malloc_hook_active = 0;
}

TEST(MallocHooks, ReallocWithHookEnabled) {
    malloc_hook_active = 1;

    void* ptr = malloc(32);
    EXPECT_NE(ptr, nullptr);

    ptr = realloc(ptr, 64);
    EXPECT_NE(ptr, nullptr);

    free(ptr);

    malloc_hook_active = 0;
}

TEST(MallocHooks, CallocWithHookEnabled) {
    malloc_hook_active = 1;

    void* ptr = calloc(10, sizeof(int));
    EXPECT_NE(ptr, nullptr);

    free(ptr);

    malloc_hook_active = 0;
}
