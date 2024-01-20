/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ARRAY_SIZE_H
#define _LINUX_ARRAY_SIZE_H

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#endif  /* _LINUX_ARRAY_SIZE_H */