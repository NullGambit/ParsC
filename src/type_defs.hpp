#pragma once

typedef unsigned char u8;
typedef signed char i8;
typedef signed short int i16;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef signed int i32;
typedef float f32;
typedef double f64;

#if __WORDSIZE == 64
typedef signed long int i64;
typedef unsigned long int u64;
#else
__extension__ typedef signed long long int i64;
__extension__ typedef unsigned long long int u64;
#endif