#pragma once
#include <stdint.h>

//Intentional crash is assert fails
#define assert(x) \
	if(!(x))  {*(int*)0 = 0;}

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

static_assert(sizeof(float) == 4, "float needs to be 4 bytes");
static_assert(sizeof(double) == 8, "double needs to be 8 bytes");

typedef float f32;
typedef double f64;
