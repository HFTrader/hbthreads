#pragma once

#include <stddef.h>
#include <stdio.h>

//! Short story - I was tired of segfaults on the malloc_hook because of the non-reentrant
//! vsprintf() and bit the bullet to create a stream that does not allocate ever

template <size_t BUFSIZE>
struct BufferPrinter {
    BufferPrinter() : _ptr(_org) {
    }
    char _org[BUFSIZE];
    char* _ptr;
    size_t size() const {
        return _ptr - _org;
    }
    int write(int fd) {
        return ::write(fd, _org, size());
    }
    int print() {
        return write(fileno(stdout));
    }
    int printerr() {
        return write(fileno(stderr));
    }
    friend BufferPrinter& operator<<(BufferPrinter& out, const char* str) {
        int nb = ::strlen(str);
        memcpy(out._ptr, str, nb);
        out._ptr += nb;
        return out;
    }
    template <size_t N>
    friend BufferPrinter& operator<<(BufferPrinter& out, const char (&str)[N]) {
        memcpy(out._ptr, str, N - 1);
        out._ptr += N - 1;
        return out;
    }
    static int digit(int ch) {
        return ch < 10 ? '0' + ch : 'a' + (ch - 10);
    }
    template <typename T>
    void printhex(const T& x) {
        constexpr size_t N = sizeof(T) * 2;  // 2 chars per byte
        char buf[N];
        T value = x;
        for (int j = 0; j < N; ++j) {
            _ptr[N - 1 - j] = digit(value % 16);
            value /= 16;
        }
        _ptr += N;
    }
    friend BufferPrinter& operator<<(BufferPrinter& out, uint16_t value) {
        out << "0x";
        out.printhex(value);
        return out;
    }
    friend BufferPrinter& operator<<(BufferPrinter& out, uint32_t value) {
        if (uint16_t(value) == value) return out << uint16_t(value);
        out << "0x";
        out.printhex(value);
        return out;
    }
    friend BufferPrinter& operator<<(BufferPrinter& out, uint64_t value) {
        if (uint32_t(value) == value) return out << uint32_t(value);
        out << "0x";
        out.printhex(value);
        return out;
    }
    friend BufferPrinter& operator<<(BufferPrinter& out, void* ptr) {
        return out << reinterpret_cast<uint64_t>(ptr);
    }
    template <typename T>
    friend BufferPrinter& operator<<(BufferPrinter& out, T const* const ptr) {
        return out << reinterpret_cast<uint64_t>(ptr);
    }
};