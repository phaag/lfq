/*
  Copyright (c) 2018,2019, Ruslan Nikolaev
  All rights reserved.
*/

/* vi: set tabstop=4: */

#ifndef __BITS_LF_H
#define __BITS_LF_H	1

#include <inttypes.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>


// PH: included LFCONGIG_H

/* For the following architectures, it is cheaper to use split (word-atomic)
   loads whenever possible. */
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__) ||	\
	defined(__aarch64__)
# define __LFLOAD_SPLIT(dtype_width)	(dtype_width > LFATOMIC_WIDTH)
#else
# define __LFLOAD_SPLIT(dtype_width)	0
#endif

/* IA-64 provides a 128-bit single-compare/double-swap instruction, so
   LFCMPXCHG_SPLIT is true for 128-bit types. */
#if defined(__ia64__)
# define __LFCMPXCHG_SPLIT(dtype_width)	(dtype_width > LFATOMIC_WIDTH)
#else
# define __LFCMPXCHG_SPLIT(dtype_width)	0
#endif

#if defined(__x86_64__) || defined (__aarch64__) || defined(__powerpc64__)	\
	|| (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI64)
typedef uint64_t lfepoch_t;
typedef int64_t lfepoch_signed_t;
typedef int64_t lfsatomic_t;
typedef uint64_t lfatomic_t;
typedef __uint128_t lfatomic_big_t;
# define LFATOMIC_LOG2			3
# define LFATOMIC_WIDTH			64
# define LFATOMIC_BIG_WIDTH		128
#elif defined(__i386__) || defined(__arm__) || defined(__powerpc__)			\
	|| (defined(__mips__) &&												\
		(_MIPS_SIM == _MIPS_SIM_ABI32 || _MIPS_SIM == _MIPS_SIM_NABI32))
# if defined(__i386__)
typedef uint64_t lfepoch_t; /* Still makes sense to use 64-bit numbers. */
typedef int64_t lfepoch_signed_t;
# else
typedef uint32_t lfepoch_t;
typedef int32_t lfepoch_signed_t;
# endif
typedef int32_t lfsatomic_t;
typedef uint32_t lfatomic_t;
typedef uint64_t lfatomic_big_t;
# define LFATOMIC_LOG2			2
# define LFATOMIC_WIDTH			32
# define LFATOMIC_BIG_WIDTH		64
#else
typedef uintptr_t lfepoch_t;
typedef intptr_t lfsatomic_t;
typedef uintptr_t lfatomic_t;
typedef uintptr_t lfatomic_big_t;
# if UINTPTR_MAX == UINT32_C(0xFFFFFFFF)
#  define LFATOMIC_LOG2			2
#  define LFATOMIC_WIDTH		32
#  define LFATOMIC_BIG_WIDTH	32
# elif UINTPTR_MAX == UINT64_C(0xFFFFFFFFFFFFFFFF)
#  define LFATOMIC_LOG2			3
#  define LFATOMIC_WIDTH		64
#  define LFATOMIC_BIG_WIDTH	64
# endif
#endif

/* XXX: True for x86/x86-64 but needs to be properly defined for other CPUs. */
#define LF_CACHE_SHIFT		7U
#define LF_CACHE_BYTES		(1U << LF_CACHE_SHIFT)

/* Allow to use LEA for x86/x86-64. */
#if defined(__i386__) || defined(__x86_64__)
# define __LFMERGE(x,y)	((x) + (y))
#else
# define __LFMERGE(x,y)	((x) | (y))
#endif

#ifdef __GNUC__
# define __LF_ASSUME(c) do { if (!(c)) __builtin_unreachable(); } while (0)
#else
# define __LF_ASSUME(c)
#endif

/* GCC does not have a sane implementation of wide atomics for x86-64
   in recent versions, so use inline assembly workarounds whenever possible.
   No aarch64 support in GCC for right now. */
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) &&	\
	!defined(__llvm__) && defined(__GCC_ASM_FLAG_OUTPUTS__)
# include "gcc_x86.h"
#else
# include "c11.h"
#endif

/* ABA tagging with split (word-atomic) load/cmpxchg operation. */
#if __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 1 ||								\
		__LFCMPXCHG_SPLIT(LFATOMIC_BIG_WIDTH) == 1
# define __LFABA_IMPL(w, type_t)											\
static const size_t __lfaba_shift##w = sizeof(lfatomic_big_t) * 4;			\
static const size_t __lfaptr_shift##w = 0;									\
static const lfatomic_big_t __lfaba_mask##w =								\
				~(lfatomic_big_t) 0U << (sizeof(lfatomic_big_t) * 4);		\
static const lfatomic_big_t __lfaba_step##w =								\
				(lfatomic_big_t) 1U << (sizeof(lfatomic_big_t) * 4);
#endif

/* ABA tagging when load/cmpxchg is not split. Note that unlike previous
   case, __lfaptr_shift is required to be 0. */
#if __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 0 &&								\
		__LFCMPXCHG_SPLIT(LFATOMIC_BIG_WIDTH) == 0
# define __LFABA_IMPL(w, type_t)											\
static const size_t __lfaba_shift##w = sizeof(type_t) * 8;					\
static const size_t __lfaptr_shift##w = 0;									\
static const lfatomic_big_t __lfaba_mask##w =								\
				~(lfatomic_big_t) 0U << (sizeof(type_t) * 8);				\
static const lfatomic_big_t __lfaba_step##w =								\
				(lfatomic_big_t) 1U << (sizeof(type_t) * 8);
#endif

typedef bool (*lf_check_t) (void * data, void * addr, size_t size);

static inline size_t lf_pow2(size_t order)
{
	return (size_t) 1U << order;
}

static inline bool LF_DONTCHECK(void * head, void * addr, size_t size)
{
	return true;
}

#define lf_container_of(addr, type, field)									\
	(type *) ((char *) (addr) - offsetof(type, field))

#ifdef __cplusplus
# define LF_ERROR	UINTPTR_MAX
#else
# define LF_ERROR	((void *) UINTPTR_MAX)
#endif

/* Available on all 64-bit and CAS2 32-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 64
__LFABA_IMPL(32, uint32_t)
#endif

/* Available on CAS2 64-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 128
__LFABA_IMPL(64, uint64_t)
#endif

/* Available on CAS2 32/64-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 2 * __LFPTR_WIDTH
__LFABA_IMPL(, uintptr_t)
#endif

#endif	/* !__BITS_LF_H */

/* vi: set tabstop=4: */
