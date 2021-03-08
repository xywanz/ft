#ifndef _LOCKFREEQUEUE_RING_H_
#define _LOCKFREEQUEUE_RING_H_

#include <stdint.h>

#include "ft/utils/lockfree-queue/misc.h"

#define CACHE_ALIGNED   __attribute__((aligned(64)))

#define LFRING_MAX_BUSYLOOP_COUNT   2
#define LFRING_INVALID_ID           0xffffffffUL
#define LFRING_MASK_LOW32           0xffffffffUL

#ifdef __cplusplus
extern "C" {
#endif

static __always_inline uint32_t upper_power_of_two(uint32_t x)
{
    --x;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x + 1;
}

typedef atomic_uint_fast64_t   LFRingNode;

/*
        LFRing是一个循环队列，LFRing里保存的只是实际数据的索引号。
        索引号可以看作是对一维数组的索引，实际的数据应该存放在一个一维数组里。在struct LFRing
    中也可以看到nodes的类型是一个64位整形的数组，其低32位存的是索引，高32位存的是当前节点序列
    号的低32位（只用于出入队时检验节点是否正确，因为多进程可能会读取到脏数据，还未检验删除这段
    逻辑是否可行⁄(⁄ ⁄•⁄ω⁄•⁄ ⁄)⁄）。
        序列号是64位整形，用于标识每一个特定的节点，解决循环队列头尾问题，head代表当前可写的
    序列号，tail代表当前可读的序列号。将序列号和mask做与操作得到当前入队或出队节点的下标。64位
    序列号暂时应该够用(`・ω・´)
        LFRing提供的操作并不是安全的操作，如果入队数量大于容量，那么会出现死循环等问题。LFRing
    提供的只是最核心的逻辑，安全管控需要在外层逻辑进行。请参考LFQueue，核心的无锁逻辑在于LFRing，
    而核心的队列设计在于LFQueue。
 */
typedef struct
{
    uint64_t node_count;
    uint64_t node_count_mask CACHE_ALIGNED;
    atomic_uint_fast64_t head_seq CACHE_ALIGNED;
    atomic_uint_fast64_t tail_seq CACHE_ALIGNED;
    LFRingNode nodes[] CACHE_ALIGNED;
} LFRing;

/*
    1. 该函数用于初始化LFRing，内存分配好后再调用该函数。
    2. count必须为2的幂，否则后果不可设想。
    3. init_head表示初始化的时候队列中有多少元素，把[0, init_head)由小到大入队。
 */
static __always_inline void LFRing_init(LFRing *ring, uint32_t count, uint32_t init_head)
{
    uint64_t i;
    ring->node_count = count;
    ring->node_count_mask = count - 1;
    ring->head_seq = init_head;
    ring->tail_seq = 0;
    for (i = 0; i < init_head; ++i)
        ring->nodes[i] = (i << 32) | i;
    for(; i < count; ++i)
        ring->nodes[i] = (i << 32) | LFRING_INVALID_ID;
}

/*
    索引入队，返回该索引的序列号
 */
static __always_inline uint64_t LFRing_push(LFRing *ring, uint32_t id)
{
    uint64_t busy_loop = 0;
    uint64_t expired_node = 0;
    uint64_t cur_head;
    uint64_t cur_node, expected_node, new_node;
    uint64_t mask = ring->node_count_mask;
    LFRingNode *atom_cur_node;
    
    for (;;) {
        cur_head = ring->head_seq;
        atom_cur_node = &ring->nodes[cur_head & mask];
        cur_node = *atom_cur_node;
        expected_node = (cur_head << 32) | LFRING_INVALID_ID;
        if (cur_node == expected_node) {
            new_node = (cur_head << 32) | id;
            if (atomic_compare_exchange_weak(atom_cur_node, &expected_node, new_node))
                break;
            expired_node = 0;
            busy_loop = 0;
            continue;
        }

/*
        若其他进程在修改node后崩溃或被调度，该进程进入忙等待，超过一定次数后直接对head进行修复，
    实际上并不需要等待即可直接修复node，此处加入忙等待次数是为了防止进程之间竞争太激烈，但不等待
    的吞吐量应该会更高。
 */
        if (expired_node == cur_node) {
            ++busy_loop;
            if (busy_loop >= LFRING_MAX_BUSYLOOP_COUNT) {
                atomic_compare_exchange_weak(&ring->head_seq, &cur_head, cur_head + 1);
                expired_node = 0;
                busy_loop = 0;
            }
        } else {
            expired_node = cur_node;
            busy_loop = 0;
        }
    }

/*
        如果当前进程执行到此处被调度或者崩溃退出，那么其他的进程会进入忙等待，
    超过忙最大忙等待次数后将会对head进行修复，不会使程序无限阻塞。
 */
    atomic_compare_exchange_weak(&ring->head_seq, &cur_head, cur_head + 1);
    return cur_head;
}

/*
    索引出队，out_seq可用于获取索引的序列号
 */
static __always_inline uint32_t LFRing_pop(LFRing *ring, int64_t *out_seq)
{
    uint64_t busy_loop = 0;
    uint64_t expired_node = 0;
    uint64_t cur_head, cur_tail;
    uint64_t cur_node, new_node;
    uint64_t mask = ring->node_count_mask;
    LFRingNode *atom_cur_node;

    for (;;) {
        __asm__ __volatile__ ("lfence" : : : "memory");
        cur_tail = ring->tail_seq;
        cur_head = ring->head_seq;
        if (cur_tail == cur_head)
            return LFRING_INVALID_ID;

        if (cur_tail > cur_head)
            continue;

        atom_cur_node = &ring->nodes[cur_tail & mask];
        cur_node = *atom_cur_node;

        if ((cur_tail & LFRING_MASK_LOW32) == (cur_node >> 32) && (cur_node & LFRING_MASK_LOW32) != LFRING_INVALID_ID) {
            new_node = ((cur_tail + ring->node_count) << 32) | LFRING_INVALID_ID;
            if (atomic_compare_exchange_weak(atom_cur_node, &cur_node, new_node))
                break;
            expired_node = 0;
            busy_loop = 0;
            continue;
        }

/*
        若其他进程在修改node后崩溃或被调度，该进程进入忙等待，超过一定次数后直接对head进行修复，
    实际上并不需要等待即可直接修复node，此处加入忙等待次数是为了防止进程之间竞争太激烈，但不等待
    的吞吐量应该会更高。
 */
        if (expired_node == cur_node) {
            ++busy_loop;
            if (busy_loop >= LFRING_MAX_BUSYLOOP_COUNT) {
                atomic_compare_exchange_weak(&ring->tail_seq, &cur_tail, cur_tail + 1);
                expired_node = 0;
                busy_loop = 0;
            }
        } else {
            expired_node = cur_node;
            busy_loop = 0;
        }
    }

/*
        如果当前进程执行到此处被调度或者崩溃退出，那么其他的进程会进入忙等待，
    超过忙最大忙等待次数后将会对head进行修复，不会使程序无限阻塞。
        进程在此处崩溃，会导致该Node丢失，可通过定期检测来修复，或是人工修复。
 */
    atomic_compare_exchange_weak(&ring->tail_seq, &cur_tail, cur_tail + 1);

    if (out_seq) {
        *out_seq = cur_tail;
    }

    return cur_node & LFRING_MASK_LOW32;
}

/*
        获取LFRing结构体的总大小，count必须为2的幂
 */
static __always_inline uint64_t LFRing_size(uint32_t count)
{
    return sizeof(LFRingNode) * count + sizeof(LFRing);
}

#ifdef __cplusplus
}
#endif

#endif
