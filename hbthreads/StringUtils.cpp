#include "StringUtils.h"
#include <string.h>

namespace hbthreads {

static void printHex(char* p, int ch) {
    *p = ch >= 10 ? 'a' + (ch - 10) : '0' + ch;
}

static void printByte(char* p, int ch) {
    printHex(p, ch / 16);
    printHex(p + 1, ch % 16);
}

void printhex(std::ostream& out, const void* data, uint32_t size,
              const std::string& line_prefix, int NUMITEMS) {
    const uint8_t* p = (const uint8_t*)data;
    int nlines = size / NUMITEMS;
    char* buf = (char*)alloca(5 * NUMITEMS + 6);
    const int LINELEN = 5 * NUMITEMS + 1;
    const int OFFSET = 3 * NUMITEMS + 1;
    buf[LINELEN] = '\n';
    buf[LINELEN + 1] = '\0';
    for (int i = 0; i < nlines; ++i) {
        char* pb = buf;
        char* po = buf + OFFSET;
        memset(buf, ' ', LINELEN);
        for (int j = 0; j < NUMITEMS; ++j) {
            int ch = *p++;
            printByte(&pb[3L * j], ch);
            po[j] = isprint(ch) ? ch : '.';
        }
        char addr[16];
        ::sprintf(addr, "%04x  ", i * NUMITEMS);
        out << line_prefix << addr << std::string(buf, LINELEN + 1);
    }
    int rem = size % NUMITEMS;
    if (rem > 0) {
        char* pb = buf;
        char* po = buf + OFFSET;
        memset(buf, ' ', LINELEN);
        for (int j = 0; j < rem; ++j) {
            int ch = *p++;
            printByte(&pb[3L * j], ch);
            po[j] = isprint(ch) ? ch : '.';
        }
        char addr[16];
        ::sprintf(addr, "%04x  ", nlines * NUMITEMS);
        out << line_prefix << addr << std::string(buf, LINELEN + 1);
    }
}

}  // namespace hbthreads