/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TOOLS_K_LINUX_TYPES_H_
#define _TOOLS_K_LINUX_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef __SANE_USERSPACE_TYPES__
#define __SANE_USERSPACE_TYPES__	/* For PPC64, to get LL64 types */
#endif

// #include <asm/types.h>
// #include <asm/posix_types.h>
#include "int-ll64.h"

struct page;
struct kmem_cache;

typedef enum {
	GFP_KERNEL,
	GFP_ATOMIC,
	__GFP_HIGHMEM,
	__GFP_HIGH
} gfp_t;

/*
 * We define u64 as uint64_t for every architecture
 * so that we can print it with "%"PRIx64 without getting warnings.
 *
 * typedef __u64 u64;
 * typedef __s64 s64;
 */
typedef uint64_t u64;
typedef int64_t s64;

typedef uint32_t u32;
typedef int32_t s32;

typedef uint16_t u16;
typedef int16_t s16;

typedef uint8_t  u8;
typedef int8_t  s8;

#ifndef __bitwise
#ifdef __CHECKER__
#define __bitwise	__attribute__((bitwise))
#else
#define __bitwise
#endif
#endif 

#define __force
/* This is defined in linux/compiler_types.h and is left for backward
 * compatibility.
 */
#ifndef __user
#define __user
#endif
#define __must_check
#define __cold

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif

typedef struct {
	int counter;
} atomic_t;

typedef struct {
	long counter;
} atomic_long_t;

#ifndef __aligned_u64
# define __aligned_u64 __u64 __attribute__((aligned(8)))
#endif

typedef struct k_list_head {
	struct k_list_head *next, *prev;
} k_list_head_s;

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#endif /* _TOOLS_K_LINUX_TYPES_H_ */