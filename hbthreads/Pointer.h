// Pointer.h - Intrusive smart pointer implementation with custom memory management
//
// This header provides an intrusive smart pointer system designed for high-performance
// applications. The Pointer<T> class wraps boost::intrusive_ptr and provides automatic
// reference counting for objects inheriting from ObjectCounter.
//
// Key features:
// - Zero-overhead reference counting using intrusive counters
// - Custom memory allocation with size embedding for efficient deallocation
// - Thread-local memory pool support for allocation performance
// - Hash map/set support through std::hash specialization
// - Configurable counter and size types for memory optimization

#pragma once
#include "ImportedTypes.h"

namespace hbthreads {

// Configuration macros for optimizing memory usage in small objects
// These can be enabled via CMake: -DUSE_SMALL_COUNTER=ON -DUSE_SMALL_SIZE=ON
#ifdef USE_SMALL_COUNTER
#define IntrusiveCounterType std::uint16_t
#else
#define IntrusiveCounterType std::uint32_t
#endif

#ifdef USE_SMALL_SIZE
#define IntrusiveSizeType std::uint16_t
#else
#define IntrusiveSizeType std::uint32_t
#endif

// Thread-local memory storage for custom allocation
// Must be initialized before allocating any ObjectCounter subclasses
extern __thread MemoryStorage* storage;

// Intrusive smart pointer template providing automatic reference counting
// and hash container support. Objects must inherit from ObjectCounter.
template <typename T>
class Pointer : public IntrusivePointer<T> {
public:
    // Default constructor - creates a null pointer
    Pointer() {
    }

    // Constructor taking ownership of a raw pointer
    // The pointed-to object must inherit from ObjectCounter
    Pointer(T* ptr) : IntrusivePointer<T>(ptr) {
    }

    // Equality comparison for use in containers
    // Compares the underlying raw pointers for identity
    bool operator==(const Pointer<T>& rhs) const noexcept {
        return this->get() == rhs.get();
    }
};

// Base class providing intrusive reference counting functionality
// All objects managed by Pointer<T> must inherit from this class.
// Uses custom operator new/delete with size embedding for efficient deallocation.
class ObjectCounter {
protected:
    // Protected constructor prevents direct instantiation
    // Initializes reference counter to 0
    ObjectCounter() : _counter(0) {
    }

public:
    // Virtual destructor for polymorphic deletion
    virtual ~ObjectCounter() noexcept {
    }

    // Custom operator new using thread-local memory storage
    // Embeds allocation size before the returned pointer for deallocation
    void* operator new(size_t size) {
        // CRITICAL: Ensure storage is initialized before allocating objects
        assert(storage != nullptr &&
               "Thread-local storage must be initialized before allocating Object "
               "subclasses");

        // This will catch bad things in debug mode
        assert(size < std::numeric_limits<IntrusiveSizeType>::max());
        assert(_library_counter_size == sizeof(IntrusiveCounterType));
        assert(_library_chunk_size == sizeof(IntrusiveSizeType));

        // Allocates oversized space
        uint8_t* ptr = (uint8_t*)storage->allocate(size + sizeof(IntrusiveSizeType));
        // Embed the counter
        *((IntrusiveSizeType*)ptr) = size;
        // Returns pointer to after the counter
        return ptr + sizeof(IntrusiveSizeType);
    }

    // Custom operator delete for deallocation
    // Retrieves embedded size and calls storage deallocator
    void operator delete(void* p) noexcept {
        // Gets pointer to the original space
        uint8_t* ptr = ((uint8_t*)p) - sizeof(IntrusiveSizeType);
        // Retrieves the size
        size_t size = *((IntrusiveSizeType*)ptr);
        // Calls the memory storage deallocator with the pointer and size
        storage->deallocate(ptr, size);
    }

private:
    // Friend functions for boost::intrusive_ptr reference counting
    friend void intrusive_ptr_add_ref(ObjectCounter*);

    // Friend functions for boost::intrusive_ptr reference counting
    friend void intrusive_ptr_release(ObjectCounter*);

    // Reference counter for intrusive pointer management
    IntrusiveCounterType _counter;

    // Static sanity check variables
    static std::uint32_t _library_counter_size;
    static std::uint32_t _library_chunk_size;
};

// Boost intrusive_ptr support function - increments reference count
inline void intrusive_ptr_add_ref(ObjectCounter* p) noexcept {
    p->_counter += 1;
}

// Boost intrusive_ptr support function - decrements reference count and deletes when 0
inline void intrusive_ptr_release(ObjectCounter* p) noexcept {
    if (p->_counter > 1) {
        p->_counter -= 1;
        return;
    }
    delete p;
}

// Concrete base class for objects managed by intrusive pointers
// Uses virtual inheritance to prevent diamond problem in multiple inheritance scenarios
class Object : public virtual ObjectCounter {
    // Inherit constructors from ObjectCounter
    using ObjectCounter::ObjectCounter;
};

}  // namespace hbthreads

namespace std {

// Hash specialization for Pointer<T> to enable use as keys in unordered containers
// Uses pointer identity for hashing (not object content)
template <typename T>
struct hash<hbthreads::Pointer<T>> {
    typedef std::size_t result_type;
    std::size_t operator()(const hbthreads::Pointer<T>& rhs) const noexcept {
        return reinterpret_cast<std::size_t>(rhs.get());
    }
};

};  // namespace std
