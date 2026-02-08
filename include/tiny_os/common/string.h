#pragma once

#include <tiny_os/common/types.h>

namespace tiny_os {

// String manipulation functions (no std library available)

usize strlen(const char* str);
void* memset(void* dest, int ch, usize count);
void* memcpy(void* dest, const void* src, usize count);
int memcmp(const void* ptr1, const void* ptr2, usize count);
char* strcpy(char* dest, const char* src);
int strcmp(const char* str1, const char* str2);

// Number to string conversion
void itoa(int64 value, char* buffer, int base = 10);
void utoa(uint64 value, char* buffer, int base = 10);

} // namespace tiny_os
