#pragma once

#include <stddef.h>
#include <stdio.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

#ifdef NDEBUG
#define debug(fmt, args...)
#define debug_r(fmt, args...)
#else
#define debug(fmt, args...) printf(fmt "\n", ##args)            // output happens util a '\n'
#define debug_r(fmt, args...) fprintf(stderr, fmt, ##args)      // output immediately
#endif
