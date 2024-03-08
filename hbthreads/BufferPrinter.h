#pragma once

#include <stddef.h>
#include <stdio.h>

//! Short story - I was tired of segfaults on the malloc_hook because of the non-reentrant
//! vsprintf() and bit the bullet to create a stream that does not allocate ever

template <size_t BUFSIZE>
struct BufferPrinter {
    //! Holds its own char buffer. The intent is to create this object in the stack
    char _org[BUFSIZE];

    //! Initializes to a single post
    BufferPrinter() : _ptr(_org) {
    }

    //! Current write pointer
    char* _ptr;

    //! Number of already written bytes
    size_t size() const {
        return _ptr - _org;
    }

    //! Helper - writes buffer to the given file descriptor
    int write(int fd) {
        return ::write(fd, _org, size());
    }

    //! Helper - writes buffer to stdout
    int print() {
        return write(fileno(stdout));
    }

    //! Helper - writes buffer to stderr
    int printerr() {
        return write(fileno(stderr));
    }

    //! Saves the given string into the buffer. This will match pointers only
    friend BufferPrinter& operator<<(BufferPrinter& out, const char* str) {
        int nb = ::strlen(str);
        memcpy(out._ptr, str, nb);
        out._ptr += nb;
        return out;
    }

    //! Saves the given string into the buffer. This will match literal char arrays.
    template <size_t N>
    friend BufferPrinter& operator<<(BufferPrinter& out, const char (&str)[N]) {
        memcpy(out._ptr, str, N - 1);
        out._ptr += N - 1;
        return out;
    }

    //! Stream/print 16 bit integers
    friend BufferPrinter& operator<<(BufferPrinter& out, uint16_t value) {
        out << "0x";
        out.printhex(value);
        return out;
    }

    //! Stream/print 32 bit integers - but will print as 16 if small enough
    friend BufferPrinter& operator<<(BufferPrinter& out, uint32_t value) {
        if (uint16_t(value) == value) return out << uint16_t(value);
        out << "0x";
        out.printhex(value);
        return out;
    }

    //! Stream/print 64 bit integers - but will print as 32 if small enough
    friend BufferPrinter& operator<<(BufferPrinter& out, uint64_t value) {
        if (uint32_t(value) == value) return out << uint32_t(value);
        out << "0x";
        out.printhex(value);
        return out;
    }

    //! Stream/prints a pointer
    friend BufferPrinter& operator<<(BufferPrinter& out, void* ptr) {
        return out << reinterpret_cast<uint64_t>(ptr);
    }

    //! Stream/prints an object pointer
    template <typename T>
    friend BufferPrinter& operator<<(BufferPrinter& out, T const* const ptr) {
        return out << reinterpret_cast<uint64_t>(ptr);
    }

private:
    //! Helper - returns the ascii digit given the value
    static int digit(int ch) {
        return ch < 10 ? '0' + ch : 'a' + (ch - 10);
    }

    //! Very trivial asciifier of an integer value
    template <typename T>
    void printhex(const T& x) {
        constexpr size_t N = sizeof(T) * 2;  // 2 chars per byte
        T value = x;
        for (size_t j = 0; j < N; ++j) {
            _ptr[N - 1 - j] = digit(value % 16);
            value /= 16;
        }
        _ptr += N;
    }
};