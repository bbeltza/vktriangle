// This is a slightly modified version of one of my utility headers from my game engine that I stole hahahah (utils/mem.h)

#ifndef UT_MEM_H
#define UT_MEM_H


// Casting macros for language-agnostic headers (To avoid C style casting in C++ while also providing C compatibility)
#ifdef __cplusplus
	#define ut_reinterpret_cast(t, x) reinterpret_cast<t>(x)
	#define ut_dynamic_cast(t, x) dynamic_cast<t>(x)
	#define ut_static_cast(t, x) static_cast<t>(x)
	#define ut_const_cast(t, x) const_cast<t>(x)
#else
	#define __ut_cast(t, x) (t)(x)
	#define ut_reinterpret_cast __ut_cast
	#define ut_dynamic_cast __ut_cast
	#define ut_static_cast __ut_cast
	#define ut_const_cast __ut_cast
#endif

/*
Creates a null terminated array, of anything, takes any argument to put into an array in which will add NULL at the end, to be able to iterate without having to speficy a size.

It's discouraged to use it with numbers as numbers can be 0
*/
#define ut_array(...) {__VA_ARGS__, NULL}
#define ut_arrcount(arr) (sizeof(arr) / sizeof(arr[0]))

#if defined(_MSC_VER) && !defined(__clang__)
	#include <malloc.h>
	#define __ut_dynsalloc(t, x, c) t *(x) = ut_static_cast(t*, alloca((c) * sizeof(t)))
#else
	#define __ut_dynsalloc(t, x, c) t x[c]
#endif

// Dynamically allocates an array (x) of c elements of type t, on the stack.
// Depending on the implementation, it may support variable length arrays (e.g. clang & gcc's extensions), or just use alloca
#define ut_dynsalloc __ut_dynsalloc

// You still need to include <string.h> for memset or have memset
#define ut_zero(x) memset(&(x), 0, sizeof(x))

#endif