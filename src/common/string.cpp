#include <tiny_os/common/string.h>

namespace tiny_os {

usize strlen(const char* str) {
    usize len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void* memset(void* dest, int ch, usize count) {
    auto* d = static_cast<uint8*>(dest);
    for (usize i = 0; i < count; i++) {
        d[i] = static_cast<uint8>(ch);
    }
    return dest;
}

void* memcpy(void* dest, const void* src, usize count) {
    auto* d = static_cast<uint8*>(dest);
    auto* s = static_cast<const uint8*>(src);
    for (usize i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, usize count) {
    auto* p1 = static_cast<const uint8*>(ptr1);
    auto* p2 = static_cast<const uint8*>(ptr2);
    for (usize i = 0; i < count; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

char* strcpy(char* dest, const char* src) {
    char* orig_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return orig_dest;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

void itoa(int64 value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        buffer[0] = '\0';
        return;
    }

    char* ptr = buffer;
    bool negative = false;

    if (value < 0 && base == 10) {
        negative = true;
        value = -value;
    }

    // Convert to unsigned and use utoa
    utoa(static_cast<uint64>(value), ptr, base);

    // Add negative sign if needed
    if (negative) {
        usize len = strlen(ptr);
        for (usize i = len; i > 0; i--) {
            ptr[i] = ptr[i - 1];
        }
        ptr[0] = '-';
    }
}

void utoa(uint64 value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        buffer[0] = '\0';
        return;
    }

    const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char temp[65];  // Max length for binary representation
    int pos = 0;

    // Handle 0 specially
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    // Convert digits in reverse order
    while (value > 0) {
        temp[pos++] = digits[value % base];
        value /= base;
    }

    // Reverse the string
    for (int i = 0; i < pos; i++) {
        buffer[i] = temp[pos - 1 - i];
    }
    buffer[pos] = '\0';
}

} // namespace tiny_os
