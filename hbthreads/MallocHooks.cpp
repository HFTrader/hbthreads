// MallocHooks.cpp - Memory allocation debugging hooks
//
// This file implements debugging hooks for intercepting and logging memory
// allocation calls (malloc, free, calloc, realloc). When enabled, it provides
// detailed tracing of memory operations including allocation sizes, addresses,
// and caller information.
//
// The hooks use weak symbol declarations to allow GLIBC to override them
// during static linking. The caller address is captured using GCC's
// __builtin_frame_address() for debugging purposes.

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "BufferPrinter.h"

extern "C" void* __libc_malloc(size_t size);     // NOLINT(bugprone-reserved-identifier)
extern "C" void __libc_free(void*);              // NOLINT(bugprone-reserved-identifier)
extern "C" void* __libc_calloc(size_t, size_t);  // NOLINT(bugprone-reserved-identifier)
extern "C" void* __libc_realloc(void*, size_t);  // NOLINT(bugprone-reserved-identifier)

// Inform the linker that the malloc/free/realloc symbols here are weak
// so they are overwritten by the GLIBC symbols in case of static linking
#pragma weak malloc
#pragma weak free
#pragma weak realloc

// Set this to "1" in main to get the printouts
int malloc_hook_active = 0;

// Hook function for malloc calls - logs allocation details and caller
static void* malloc_hook(size_t size, void* caller) {
    void* result;
    malloc_hook_active = 0;
    result = malloc(size);
    BufferPrinter<64> bf;
    bf << "malloc(" << size << ")=" << result << " Caller:" << caller << "\n";
    bf.printerr();
    malloc_hook_active = 1;
    return result;
}

// Hook function for free calls - logs deallocation details and caller
static void free_hook(void* ptr, void* caller) {
    malloc_hook_active = 0;
    BufferPrinter<64> bf;
    bf << "free(" << ptr << ") caller:" << caller << "\n";
    bf.printerr();
    free(ptr);
    malloc_hook_active = 1;
}

// Hook function for calloc calls - logs allocation details and caller
static void* calloc_hook(size_t nmemb, size_t size, void* caller) {
    malloc_hook_active = 0;
    void* result = calloc(nmemb, size);
    BufferPrinter<64> bf;
    bf << "calloc(" << nmemb << "," << size << ") = " << result << "  caller:" << caller
       << "\n";
    bf.printerr();
    malloc_hook_active = 1;
    return result;
}

// Hook function for realloc calls - logs reallocation details and caller
static void* realloc_hook(void* ptr, size_t size, void* caller) {
    malloc_hook_active = 0;
    void* result = realloc(ptr, size);
    BufferPrinter<64> bf;
    bf << "realloc(" << ptr << "," << size << ") = " << result << "  caller:" << caller
       << "\n";
    bf.printerr();
    malloc_hook_active = 1;
    return result;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
extern "C" void* malloc(size_t size) {
    void* caller = __builtin_frame_address(1);
    if (malloc_hook_active) {
        return malloc_hook(size, caller);
    }
    return __libc_malloc(size);
}

extern "C" void free(void* ptr) {
    void* caller = __builtin_frame_address(1);
    if (malloc_hook_active) {
        free_hook(ptr, caller);
        return;
    }
    __libc_free(ptr);
}

extern "C" void* calloc(size_t nmemb, size_t size) {
    void* caller = __builtin_frame_address(1);
    if (malloc_hook_active) {
        return calloc_hook(nmemb, size, caller);
    }
    return __libc_calloc(nmemb, size);
}

extern "C" void* realloc(void* ptr, size_t size) {
    void* caller = __builtin_frame_address(1);
    if (malloc_hook_active) {
        return realloc_hook(ptr, size, caller);
    }
    return __libc_realloc(ptr, size);
}
#pragma GCC diagnostic pop
