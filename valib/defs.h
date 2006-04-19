/*
  Basic data types, constants and utility finctions
*/

#ifndef DEFS_H
#define DEFS_H

#include <basetyps.h>
#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////
// Basic constants
///////////////////////////////////////////////////////////////////////////////

#define NCHANNELS 6

#ifndef M_PI
  #define M_PI 3.1415926535897932384626433832795029
#endif

///////////////////////////////////////////////////////////////////////////////
// Level multipliers
///////////////////////////////////////////////////////////////////////////////

#define LEVEL_PLUS6DB 2.0
#define LEVEL_PLUS3DB 1.4142135623730951
#define LEVEL_3DB     0.7071067811865476
#define LEVEL_45DB    0.5946035575013605
#define LEVEL_6DB     0.5


///////////////////////////////////////////////////////////////////////////////
// Audio sample
///////////////////////////////////////////////////////////////////////////////

#ifndef FLOAT_SAMPLE

  typedef double   sample_t;

#else

  typedef float    sample_t;

  #if _MSC_VER >= 1200
    // warning C4244: '+=' : conversion from 'double' to 'float', possible loss of data
    // warning C4305: 'initializing' : truncation from 'const double' to 'const float'
    #pragma warning (disable: 4244 4305)
  #endif

#endif

#define SAMPLE_THRESHOLD (1e-10)
#define EQUAL_SAMPLES(s1, s2) (fabs(s1 - s2) < SAMPLE_THRESHOLD)

///////////////////////////////////////////////////////////////////////////////
// Bitstream types
///////////////////////////////////////////////////////////////////////////////

#define BITSTREAM_8     0
#define BITSTREAM_16BE  1
#define BITSTREAM_16LE  2
#define BITSTREAM_32BE  3
#define BITSTREAM_32LE  4
#define BITSTREAM_14BE  5  // DTS bitstream
#define BITSTREAM_14LE  6  // DTS bitstream

///////////////////////////////////////////////////////////////////////////////
// GCC vs MSVC
///////////////////////////////////////////////////////////////////////////////

#ifdef GCC
#define __forceinline inline
#endif

///////////////////////////////////////////////////////////////////////////////
// Base word types
///////////////////////////////////////////////////////////////////////////////

typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed __int64   int64_t;
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned __int64 uint64_t;

#pragma pack(push, 1)   // do not justify following structure
struct int24_t // int24_t is a low-endian structure
{
  uint16_t low; 
  int8_t   high;

  int24_t() {}
  int24_t(int32_t i)
  {
    low  = uint16_t(i & 0xFFFF);
    high = int8_t(i >> 16);
  }
  int24_t(double d)
  {
    int i = int(d);
    low  = uint16_t(i & 0xFFFF);
    high = int8_t(i >> 16);
  }
  operator int32_t()
  {
    return (high << 16) + low;
  }
};
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Other type definitions
///////////////////////////////////////////////////////////////////////////////

typedef double vtime_t;
typedef sample_t matrix_t[NCHANNELS][NCHANNELS];

///////////////////////////////////////////////////////////////////////////////
// Some utilities
//
//   MIN(a, b)           finds minimum
//   MAX(a, b)           finds maximum
//   value2db(value)     convert to decibels (requires math.h)
//   db2value(db)        convert from decibels (requires math.h)
//   array_size(array)   calculates array size
///////////////////////////////////////////////////////////////////////////////

#ifndef MIN
  #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
  #define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#define value2db(value) ((value > 0)? log10(value)*20.0: 0)
#define db2value(db)    pow(10.0, db/20.0)

#define array_size(array) (sizeof(array) / sizeof(array[0]))

///////////////////////////////////////////////////////////////////////////////
// Byte-swap functions
//
//   uint32_t swab_u32(uint32_t i)
//   int32_t  swab_s32(int32_t i)
//   uint16_t swab_u16(uint16_t i)
//   int16_t  swab_s16(int16_t i)
//   int32_t  swab_s24(int24_t i)
//
// We use defines for debug version because it works much faster than inline
// functions (overhead of function call is too big). So we must be careful
// with it because following constructions are prohibited:
//
// i = swab_u16(*p++); // WRONG!!! because it is replaced with
//                     // i = ((*p++) << 8) | (*p++) >> 8));
//
// Use asm inlines for X86 architecture and general inlines otherwise.
///////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG)

  // do not use inline functions for debug version (faaaaster)
  #define swab_u32(i) uint32_t((uint32_t(i) >> 24) | (uint32_t(i) >> 8) & 0xff00 | (uint32_t(i) << 8) & 0xff0000 | (uint32_t(i) << 24))
  #define swab_s32(i) int32_t(swab_u32(i))
  #define swab_u16(i) uint16_t((uint16_t(i) << 8) | (uint16_t(i) >> 8))
  #define swab_s16(i) int16_t(swab_u16(i))
  inline int32_t  swab_s24(int24_t i)  { return swab_s32(i) >> 8; }

#elif defined(_M_IX86) & defined(GCC)

  inline uint32_t swab_u32(uint32_t x)
  {
    asm("bswap %0" : "=r" (x) : "0" (x));
    return x;
  };
  
  inline int32_t swab_s32(int32_t x)
  {
    asm("bswap %0" : "=r" (x) : "0" (x));
    return x;
  }
  inline uint16_t swab_u16(uint16_t x)
  {
    asm("bswap %0; shr %0, 16" : "=r" (x) : "0" (x));
    return x;
  }
  inline int16_t swab_s16(int16_t x)
  {
    asm("bswap %0; shr %0, 16" : "=r" (x) : "0" (x));
    return x;
  }
  inline int32_t swab_s24(int24_t x)
  {
    asm("bswap %0; sar %0, 8" : "=r" (x) : "0" (x));
    return x;
  }

#elif defined(_M_IX86)

  // use asm inline functions
  #pragma warning(push)
  #pragma warning(disable: 4035) 
  inline uint32_t swab_u32(uint32_t x) 
  {
    __asm mov eax, x
    __asm bswap eax
  }
  inline int32_t swab_s32(int32_t x) 
  {
    __asm mov eax, x
    __asm bswap eax
  }
  inline uint16_t swab_u16(uint16_t x) 
  {
    __asm mov ax, x
    __asm bswap eax
    __asm shr eax, 16
  }
  inline int16_t swab_s16(int16_t x) 
  {
    __asm mov ax, x
    __asm bswap eax
    __asm shr eax, 16
  }
  inline int32_t swab_s24(int24_t x)
  {
    __asm mov eax, x
    __asm bswap eax
    __asm sar eax, 8
  }
  #pragma warning(pop)

#else

  // use general inline functions
  inline uint32_t swab_u32(uint32_t i) { return (i >> 24) | (i >> 8) & 0xff00 | (i << 8) & 0xff0000 | (i << 24); }
  inline int32_t  swab_s32(int32_t i)  { return (int32_t)swab_u32((uint32_t)i); }
  inline uint16_t swab_u16(uint16_t i) { return (i << 8) | (i >> 8); };
  inline int16_t  swab_s16(int16_t i)  { return (int16_t)swab_u16((uint16_t)i); };
  inline int32_t  swab_s24(int24_t i)  { return swab_s32(i) >> 8; }
#endif

inline float    swab_float(float f) { uint32_t i = swab_u32(*(uint32_t *)&f); return *(float *)&i; };

///////////////////////////////////////////////////////////////////////////////
// Byteorder conversion
///////////////////////////////////////////////////////////////////////////////

#define int2be32(i) swab_s32(i)
#define int2le32(i) (i)
#define int2be24(i) swab_s24(i)
#define int2le24(i) (i)
#define int2be16(i) swab_s16(i)
#define int2le16(i) (i)

#define be2int32(i) swab_s32(i)
#define le2int32(i) (i)
#define be2int24(i) swab_s24(i)
#define le2int24(i) (i)
#define be2int16(i) swab_s16(i)
#define le2int16(i) (i)

#define uint2be32(i) swab_u32(i)
#define uint2le32(i) (i)
#define uint2be24(i) swab_u24(i)
#define uint2le24(i) (i)
#define uint2be16(i) swab_u16(i)
#define uint2le16(i) (i)

#define be2uint32(i) swab_u32(i)
#define le2uint32(i) (i)
#define be2uint24(i) swab_u24(i)
#define le2uint24(i) (i)
#define be2uint16(i) swab_u16(i)
#define le2uint16(i) (i)


///////////////////////////////////////////////////////////////////////////////
// MSVC8: disable depreciation warning
///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable: 4996)

#endif
