#pragma once

// This file is not part of the library but it is a helper for the example
// to show the allocations that are (not) done

//! Defines if the malloc(), realloc(), calloc() and free() calls should be intercepted.
//! Implemented in MallocHooks.cpp
extern int malloc_hook_active;
