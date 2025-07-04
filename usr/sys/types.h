#pragma once

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;
typedef unsigned long   usize;

typedef signed char     i8;
typedef signed short    i16;
typedef signed int      i32;
typedef signed long     i64;
typedef signed long     isize;

typedef float           f32;
typedef double          f64;

#ifdef __cplusplus
#define NULL nullptr
#else
#define NULL ((void*)0)
#endif
