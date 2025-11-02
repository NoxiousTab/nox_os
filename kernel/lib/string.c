#include <stddef.h>
#include <stdint.h>

size_t strlen(const char* s){ size_t n=0; while(s && *s++) ++n; return n; }
