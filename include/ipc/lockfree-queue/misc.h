#ifndef _LOCKFREEQUEUE_MISC_H_
#define _LOCKFREEQUEUE_MISC_H_

#ifdef __cplusplus
#include <stdint.h>
#ifndef __always_inline
#define __always_inline inline __attribute__((__always_inline__))
#endif
typedef volatile uint64_t atomic_uint_fast64_t;

__always_inline bool atomic_compare_exchange_weak(
    atomic_uint_fast64_t *ptr,
    uint64_t *val,
    uint64_t dest)
{
    uint64_t ret;
    uint64_t prev = *val;
    __asm__ __volatile__(
        "lock\n\t"
        "cmpxchgq %1, %2"
        : "=a"(ret)
        : "r"(dest), "m"(ptr), "0"(*val)
        : "memory"
    );
    return ret == prev;
}
#else
#include <stdatomic.h>
#endif

#endif
