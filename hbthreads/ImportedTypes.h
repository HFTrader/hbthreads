#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/pmr/vector.hpp>
#include <boost/container/pmr/small_vector.hpp>
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include "boost/container/pmr/monotonic_buffer_resource.hpp"
#include "boost/container/pmr/polymorphic_allocator.hpp"
#include <boost/context/detail/fcontext.hpp>
#include <boost/fiber/protected_fixedsize_stack.hpp>
#include <limits>
#include <cassert>
#include <cstdint>

//! Namespace for light coroutines and respective reactor mechanism
namespace hbthreads {

//! Tired of including those so let's put them inside this namespace
using namespace boost::context::detail;
using namespace boost::context;

//--------------------------------------------------
// Pointers
//--------------------------------------------------
template <typename T>
using IntrusivePointer = boost::intrusive_ptr<T>;

//--------------------------------------------------
// Allocators
//--------------------------------------------------

//! A base type of all memory resource types (above)
using MemoryStorage = boost::container::pmr::memory_resource;

//! An allocator that we can use with memory pools
template <typename T>
struct Allocator : public boost::container::pmr::polymorphic_allocator<T> {
    using allocator_type = boost::container::pmr::polymorphic_allocator<T>;
    Allocator(MemoryStorage *pool) : allocator_type(pool) {
    }
};

//! Convenience specialization of allocator for file descriptors mostly
using IntAllocator = Allocator<int>;

//! An (expensive) stack allocator that avoids memory corruption
//! It uses mmap and wastes one full page (4k) for protection
//! It is expensive to allocate but thereafter has no impact on peformance
using StackAllocator = boost::context::protected_fixedsize_stack;

//--------------------------------------------------
// Flat containers to avoid memory fragmentation
//--------------------------------------------------

//! This is to save us some typing
template <typename T, typename Comp = std::less<T>>
using BoostFlatSet = boost::container::flat_set<T, Comp, Allocator<T>>;

//! This wrapper will prevent the creation of an internal pool and waste time
template <typename T, typename Comp = std::less<T>>
struct FlatSet : public BoostFlatSet<T, Comp> {
    using allocator_type = typename BoostFlatSet<T, Comp>::allocator_type;
    FlatSet(MemoryStorage *pool) : BoostFlatSet<T, Comp>(allocator_type(pool)) {
    }
};

//! This is to save us some typing
template <typename Key, typename Type, typename Comp = std::less<Key>>
using BoostFlatMap =
    boost::container::flat_map<Key, Type, Comp, Allocator<std::pair<Key, Type>>>;

//! This wrapper will prevent the creation of an internal pool and waste time
template <typename Key, typename Type, typename Comp = std::less<Key>>
struct FlatMap : public BoostFlatMap<Key, Type, Comp> {
    using allocator_type = typename BoostFlatMap<Key, Type, Comp>::allocator_type;
    FlatMap(MemoryStorage *pool) : BoostFlatMap<Key, Type, Comp>(allocator_type(pool)) {
    }
};

template <typename Type, size_t N>
using BoostSmallVector = boost::container::small_vector<Type, N, Allocator<Type>>;

//! This wrapper will prevent the creation of an internal pool and waste time
template <typename Type, size_t N>
struct SmallVector : BoostSmallVector<Type, N> {
    using allocator_type = typename BoostSmallVector<Type, N>::allocator_type;
    SmallVector(MemoryStorage *pool) : BoostSmallVector<Type, N>(allocator_type(pool)) {
    }
};

//--------------------------------------------------
// Coroutines basically
// We use boost fcontext - great code in there
// The upper classes (fiber,continuation) are not so great so we just import
//   the good stuff
//--------------------------------------------------

using ThreadContext = boost::context::detail::fcontext_t;
using ReturnContext = boost::context::detail::transfer_t;

}  // namespace hbthreads
