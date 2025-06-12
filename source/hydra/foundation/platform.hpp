#pragma once

#include <stdint.h>

#if !defined(_MSC_VER)
#include <signal.h>
#endif

// Macros ////////////////////////////////////////////////////////////////

#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))

#if defined(_MSC_VER)
#define HYDRA_INLINE inline
#define HYDRA_FINLINE __forceinline
#define HYDRA_DEBUG_BREAK __debugbreak();
#define HYDRA_DISABLE_WARNING(warning_number) __pragma(warning(disable : warning_number))
#define HYDRA_CONCAT_OPERATOR(x, y) x##y
#else
#define HYDRA_INLINE inline
#define HYDRA_FINLINE always_inline
#define HYDRA_DEBUG_BREAK raise(SIGTRAP);
#define HYDRA_CONCAT_OPERATOR(x, y) x y
#endif  // MSVC

#define HYDRA_STRINGIZE(L) #L
#define HYDRA_MAKESTRING(L) HYDRA_STRINGIZE(L)
#define HYDRA_CONCAT(x, y) HYDRA_CONCAT_OPERATOR(x, y)
#define HYDRA_LINE_STRING HYDRA_MAKESTRING(__LINE__)
#define HYDRA_FILELINE(MESSAGE) __FILE__ "(" HYDRA_LINE_STRING ") : " MESSAGE

// Unique names
#define HYDRA_UNIQUE_SUFFIX(PARAM) HYDRA_CONCAT(PARAM, __LINE__)

// Native types typedefs /////////////////////////////////////////////////
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef size_t sizet;

typedef const char* cstring;

static const u64 u64_max = UINT64_MAX;
static const i64 i64_max = INT64_MAX;
static const u32 u32_max = UINT32_MAX;
static const i32 i32_max = INT32_MAX;
static const u16 u16_max = UINT16_MAX;
static const i16 i16_max = INT16_MAX;
static const u8 u8_max = UINT8_MAX;
static const i8 i8_max = INT8_MAX;