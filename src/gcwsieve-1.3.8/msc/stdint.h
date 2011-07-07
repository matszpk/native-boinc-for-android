#ifndef _MSC_STDINT_H
#define _MSC_STDINT_H

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

typedef char int_fast8_t;
typedef unsigned char uint_fast8_t;

#ifdef _WIN64
typedef __int64 int_fast32_t;
typedef unsigned __int64 uint_fast32_t;
#else
typedef int int_fast32_t;
typedef unsigned int uint_fast32_t;
#endif

#define INT32_C(x) x
#define UINT32_C(x) x##U

#define INT64_C(x) x##I64
#define UINT64_C(x) x##UI64

#define INT8_MIN (-128) 
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN  (-9223372036854775807I64 - 1)

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807I64

#define UINT8_MAX 255U
#define UINT16_MAX 65535U
#define UINT32_MAX 4294967295U
#define UINT64_MAX 18446744073709551615UI64

#ifdef _WIN64
#define INT_FAST32_MIN INT64_MIN
#define INT_FAST32_MAX INT64_MAX
#define UINT_FAST32_MAX UINT64_MAX
#else
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX
#endif

#endif /* _MSC_STDINT_H */
