#pragma once
#include <cstdint>
#include <limits>
#include <cstddef>
#include <iterator>

namespace hbthreads {

/**
 * The hook is to be added on classes such that they can be inserted
 * into index intrusive lists.
 */
template <typename Index>
struct IntrusiveIndexListHook {
    Index prev;
    Index next;
};

/**
 * The head is to be added to container classes such that they can manage
 * intrusive objects.
 */
template <typename Index>
struct IntrusiveIndexListHead {
    static constexpr Index NullIndex = std::numeric_limits<Index>::max();
    Index first = NullIndex;
    Index last = NullIndex;
    uint32_t counter = 0;
};

// Forward declaration
template <typename T, typename Index, IntrusiveIndexListHook<Index> T::*hook,
          typename Container>
class IntrusiveIndexList;

/**
 * IntrusiveIndexList iterator type
 */
template <typename T, typename Index, IntrusiveIndexListHook<Index> T::*HookType,
          typename Container>
class IntrusiveIndexListIterator {
public:
    using List = IntrusiveIndexList<T, Index, HookType, Container>;
    using Type = IntrusiveIndexListIterator<T, Index, HookType, Container>;
    using Head = IntrusiveIndexListHead<Index>;
    using Hook = IntrusiveIndexListHook<Index>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;

    IntrusiveIndexListIterator(List& alist) : list(alist) {
        prev = Head::NullIndex;
        cur = list._head.first;
    }
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)
    explicit IntrusiveIndexListIterator(List& alist, Index prev, Index current)
        : list(alist), prev(prev), cur(current) {
    }
    // NOLINTEND(bugprone-easily-swappable-parameters)
    pointer operator()() {
        return &list._items[cur];
    }
    const_pointer operator()() const {
        return &list._items[cur];
    }
    reference operator*() const {
        return list._items[cur];
    }
    pointer operator->() const {
        return &list._items[cur];
    }
    Type& operator++() {
        cur = hook().next;
        return *this;
    }
    Type operator++(int) {
        Type tmp = *this;
        cur = hook().next;
        return tmp;
    }
    Type& operator--() {
        cur = prev;
        prev = hook().prev;
        return *this;
    }
    Type operator--(int) {
        Type tmp = *this;
        cur = prev;
        prev = hook().prev;
        return tmp;
    }
    bool operator==(const Type& rhs) const {
        return cur == rhs.cur;
    }
    bool operator!=(const Type& rhs) const {
        return cur != rhs.cur;
    }

private:
    List& list;
    Index prev;
    Index cur;
    const_pointer ptr() const {
        return &list._items[cur];
    }
    pointer ptr() {
        return &list._items[cur];
    }
    Hook& hook() {
        return ptr()->*HookType;
    }
    const Hook& hook() const {
        return ptr()->*HookType;
    }
};

/**
 * Const version of IntrusiveIndexList iterator type
 */

template <typename T, typename Index, IntrusiveIndexListHook<Index> T::*HookType,
          typename Container>
class IntrusiveIndexListConstIterator {
public:
    using List = IntrusiveIndexList<T, Index, HookType, Container>;
    using Type = IntrusiveIndexListIterator<T, Index, HookType, Container>;
    using Head = IntrusiveIndexListHead<Index>;
    using Hook = IntrusiveIndexListHook<Index>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;

    IntrusiveIndexListConstIterator(const List& alist) : list(alist) {
        prev = Head::NullIndex;
        cur = list._head.first;
    }
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)
    explicit IntrusiveIndexListConstIterator(List& alist, Index prev, Index current)
        : list(alist), prev(prev), cur(current) {
    }
    // NOLINTEND(bugprone-easily-swappable-parameters)
    pointer operator()() {
        return &list._items[cur];
    }
    const_pointer operator()() const {
        return &list._items[cur];
    }
    reference operator*() const {
        return list._items[cur];
    }
    pointer operator->() const {
        return &list._items[cur];
    }
    Type& operator++() {
        cur = hook().next;
        return *this;
    }
    Type operator++(int) {
        Type tmp = *this;
        cur = hook().next;
        return tmp;
    }
    Type& operator--() {
        cur = prev;
        prev = hook().prev;
        return *this;
    }
    Type operator--(int) {
        Type tmp = *this;
        cur = prev;
        prev = hook().prev;
        return tmp;
    }
    bool operator==(const Type& rhs) const {
        return cur == rhs.ptr;
    }
    bool operator!=(const Type& rhs) const {
        return cur != rhs.ptr;
    }

private:
    const List& list;
    Index prev;
    Index cur;
    const_pointer ptr() const {
        return &list._items[cur];
    }
    pointer ptr() {
        return &list._items[cur];
    }
    Hook& hook() {
        return ptr()->*HookType;
    }
    const Hook& hook() const {
        return ptr()->*HookType;
    }
};

/**
 * Intrusive index list should not be used as a member. It is to be
 * created temporarily to manage the list and then deleted.
 */
template <typename T, typename Index, IntrusiveIndexListHook<Index> T::*HookType,
          typename ContainerType>
struct IntrusiveIndexList {
    IntrusiveIndexListHead<Index>& _head;
    ContainerType& _items;

public:
    using Head = IntrusiveIndexListHead<Index>;
    using Hook = IntrusiveIndexListHook<Index>;
    using Container = ContainerType;

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = IntrusiveIndexListIterator<T, Index, HookType, Container>;
    using const_iterator = IntrusiveIndexListConstIterator<T, Index, HookType, Container>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    static constexpr Index NullIndex = IntrusiveIndexListHead<Index>::NullIndex;
    friend class IntrusiveIndexListIterator<T, Index, HookType, Container>;
    friend class IntrusiveIndexListConstIterator<T, Index, HookType, Container>;

    IntrusiveIndexList(IntrusiveIndexListHead<Index>& head, Container& container)
        : _head(head), _items(container) {
    }

    inline iterator begin() {
        return iterator(*this);
    }
    inline const_iterator begin() const {
        return const_iterator(*this);
    }
    inline iterator end() {
        return iterator(*this, _head.last, NullIndex);
    }
    inline const_iterator end() const {
        return const_iterator(*this, _head.last, NullIndex);
    }
    inline reverse_iterator rbegin() {
        return reverse_iterator(end());
    }
    inline reverse_iterator rend() {
        return reverse_iterator(begin());
    }
    inline bool empty() {
        return _head.first == NullIndex;
    }
    inline reference front() {
        return _items[_head.first];
    }
    inline const_reference front() const {
        return _items[_head.first];
    }
    inline reference back() {
        return _items[_head.last];
    }
    inline const_reference back() const {
        return _items[_head.last];
    }
    inline reference at(Index j) {
        iterator it = begin();
        std::advance(it, j);
        return *it;
    }
    inline const_reference at(Index j) const {
        const_iterator it = begin();
        std::advance(it, j);
        return *it;
    }
    IntrusiveIndexListHook<Index>& hook(Index index) {
        return _items[index].*HookType;
    }

    template <typename Fn>
    inline iterator find(Fn&& fun) {
        for (iterator it = begin(); it != end(); ++it) {
            if (fun(*it)) {
                return it;
            }
        }
        return end();
    }

    inline void push_front(Index index) {
        Index second = _head.first;
        _head.first = index;
        hook(index).prev = NullIndex;
        hook(index).next = second;
        if (second != NullIndex) {
            hook(second).prev = _head.first;
        } else {
            _head.last = _head.first;
        }
        _head.counter += 1;
    }
    inline Index pop_front() {
        if (_head.first != NullIndex) {
            Index tmp = _head.first;
            Index second = hook(_head.first).next;
            if (second != NullIndex) {
                hook(second).prev = NullIndex;
            } else {
                _head.last = NullIndex;
            }
            _head.first = second;
            hook(tmp).next = NullIndex;
            _head.counter -= 1;
            return tmp;
        }
        return NullIndex;
    }
    inline void push_back(Index index) {
        Index penultimate = _head.last;
        _head.last = index;
        hook(_head.last).prev = penultimate;
        hook(_head.last).next = NullIndex;
        if (penultimate != NullIndex) {
            hook(penultimate).next = _head.last;
        } else {
            _head.first = _head.last;
        }
        _head.counter += 1;
    }
    inline pointer pop_back() {
        if (_head.last != NullIndex) {
            Index tmp = _head.last;
            Index penultimate = hook(_head.last).prev;
            if (penultimate != NullIndex) {
                hook(penultimate).next = NullIndex;
            } else {
                _head.first = NullIndex;
            }
            _head.last = penultimate;
            hook(tmp).prev = NullIndex;
            _head.counter -= 1;
            return tmp;
        }
        return NullIndex;
    }
    inline iterator insert(iterator pos, Index index) {
        insert(pos(), index);
        return iterator(*this, hook(index).prev, index);
    }
    inline pointer remove(iterator pos) {
        return remove(pos());
    }
    inline void clear() {
        _head.first = _head.last = NullIndex;
        _head.counter = 0;
    }
    inline Index count() const {
        return _head.counter;
    }
    inline void insert(Index where, Index index) {
        if (where != NullIndex) {
            Index prev = hook(where).prev;
            hook(index).next = where;
            hook(index).prev = prev;
            hook(where).prev = index;
            if (prev != NullIndex) {
                hook(prev).next = index;
            } else {
                _head.first = index;
            }
            _head.counter += 1;
        } else {
            push_back(index);
        }
    }
    inline Index remove(Index index) {
        Index prev = hook(index).prev;
        Index next = hook(index).next;
        if (prev != NullIndex) {
            hook(prev).next = next;
        } else {
            _head.first = next;
        }
        if (next != NullIndex) {
            hook(next).prev = prev;
        } else {
            _head.last = prev;
        }
        _head.counter -= 1;
        return next;
    }
};
}  // namespace hbthreads