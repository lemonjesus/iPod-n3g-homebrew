// may one day be replaced with references to these functions within osos

#include <stddef.h>

void* memset(void* d, int c, size_t n) {
    unsigned char* p = d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}

void* memcpy(void* d, const void* s, size_t n) {
    unsigned char* p = d;
    const unsigned char* q = s;
    while (n--) *p++ = *q++;
    return d;
}

void* memmove(void* d, const void* s, size_t n) {
    unsigned char* p = d;
    const unsigned char* q = s;
    if (p < q) {
        while (n--) *p++ = *q++;
    } else {
        p += n;
        q += n;
        while (n--) *--p = *--q;
    }
    return d;
}
