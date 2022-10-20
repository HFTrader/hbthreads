#pragma once
#include "ImportedTypes.h"
#include <iostream>

namespace hbthreads {

// If you are feeling corageous and you want to improve the performance
// for small objects, set any or both these options ON in your cmake command line
// $ cmake  -DUSE_SMALL_COUNTER=ON -DUSE_SMALL_SIZE=ON ...
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

// A per-thread memory factory
extern __thread MemoryStorage* storage;

//! Your basic intrusive pointer plus hashmap ability
template <typename T>
class Pointer : public IntrusivePointer<T> {
public:
    Pointer() {
    }
    Pointer(T* ptr) : IntrusivePointer<T>(ptr) {
    }

    //! Allows intrusive pointers to be used in hashmaps and hashsets
    bool operator==(const Pointer<T>& rhs) {
        return this->get() == rhs.get();
    }
};

//! A base class that offers the required object counter for intrusive pointers
class ObjectCounter {
protected:
    //! Starts with counter zero
    //! We dont want anyone to instantiate objects of this class so we make it protected
    ObjectCounter() : _counter(0) {
    }

public:
    virtual ~ObjectCounter() {
    }

    //! Override operator new to use our polymorphic allocator chain
    //! We need to embed the size as the first bytes as it is required by
    //! the deallocator call
    void* operator new(size_t size) {
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

    //! Override operator delete to deallocate using our polymorphic allocator
    void operator delete(void* p) {
        // Gets pointer to the original space
        uint8_t* ptr = ((uint8_t*)p) - sizeof(IntrusiveSizeType);
        // Retrieves the size
        size_t size = *((IntrusiveSizeType*)ptr);
        // Calls the memory storage deallocator with the pointer and size
        storage->deallocate(ptr, size);
    }

private:
    //! Makes this call friendly as it will have to access private _counter
    friend void intrusive_ptr_add_ref(ObjectCounter*);

    //! Makes this call friendly as it will have to access private _counter
    friend void intrusive_ptr_release(ObjectCounter*);

    //! The number of references to this object
    IntrusiveCounterType _counter;

    //! Sanity checks
    static std::uint32_t _library_counter_size;
    static std::uint32_t _library_chunk_size;
};

// Required to increment the reference count
inline void intrusive_ptr_add_ref(ObjectCounter* p) {
    p->_counter += 1;
}

// Required to decrement the reference count and deallocate the object
inline void intrusive_ptr_release(ObjectCounter* p) {
    if (p->_counter > 1) {
        p->_counter -= 1;
        return;
    }
    delete p;
}

//! The intent with the virtual keyword is to avoid the "diamond problem"
//! https://www.makeuseof.com/what-is-diamond-problem-in-cpp/
//! if you do not ever have this problem you could consider using ObjectCounter
//! as a base class
class Object : public virtual ObjectCounter {
    //! Inherit constructors if any
    using ObjectCounter::ObjectCounter;
};

}  // namespace hbthreads

//! Allows intrusive pointers to be keys in sets and maps
namespace std {
template <typename T>
struct hash<hbthreads::Pointer<T>> {
    typedef std::size_t result_type;
    std::size_t operator()(const hbthreads::Pointer<T>& rhs) const {
        return reinterpret_cast<std::size_t>(rhs.get());
    }
};
};  // namespace std
