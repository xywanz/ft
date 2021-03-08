// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef _LOCKFREEQUEUE_QUEUE_H_
#define _LOCKFREEQUEUE_QUEUE_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdbool.h>

#include "ft/utils/lockfree-queue/ring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_MAGIC 1709394

/*
        LFQueue的节点数据结构，在一个LFQueue中，所有的LFNode均为相同长度，LFNode存储的
    数据大小有限制，即max_size，max_size应该64的倍数，在初始化时函数会自动向上取64的倍数。
    size表示实际数据长度，size <= max_size。
 */
typedef struct
{
    uint64_t size;
    char data[] CACHE_ALIGNED;
} LFNode;

typedef struct
{
    uint64_t        magic;
    uint64_t        node_data_size;
    uint64_t        node_count;
    uint64_t        node_total_size;
    bool            overwrite;
    uint32_t        user_id;

    volatile bool   pause;
    int             key;

    char            aligned[] CACHE_ALIGNED;
} LFHeader;

/*
        LFQueue是一个基于共享内存的、多进程无锁的、存储定长数据的循环队列。
    程序在任意一行崩溃都不会影响该队列的正常运行。但会导致节点的丢失，相当于
    内存泄漏，可通过定期对nodes进行检查或通过命令行手动恢复。
        LFQueue基于LFRing，LFQueue中有两个Ring，一个用于管理可用节点的下标，
    另一个用于管理已入队的节点的下标。实际的节点存放于nodes中。入队有两种模式：
        在非覆盖模式中，入队时，先尝试从Resource Ring出队获取一个可用的节点下标，
    若无可用节点，则返回一个负值，若成功获取节点，将数据拷贝至节点后，将节点下标入
    队至Node Ring，即完成入队。
        在覆盖模式中，入队时，尝试从Resource Ring出队获取一个可用的节点下标，若无
    可用节点下标，则从Node Ring中出队一个节点（即最旧的可读节点），若获取失败，则
    返回一个负值（从两个队列都获取失败是正常现象，因为有多个进程在读写，重复调用入队
    即可），若成功获取，将数据拷贝至节点后，将节点下标入队至Node Ring，即完成入队。
        出队时，从Node Ring出队一个节点，若无可读节点，则返回一个负值，若有，则
    把数据拷贝完成后，将节点下标入队至Resource Ring，表示归还资源，完成出队。
        若在出队入队之间程序崩溃，会导致节点的丢失，但不会影响队列的正常运行。使用
    者们也可自行分析，一次入队或出队需要4-6次原子操作，在这几个原子操作之间崩溃，会
    使得队列崩溃吗？
 */
typedef struct
{
    LFHeader *header;
    LFRing *resc_ring;
    LFRing *node_ring;
    char *nodes;
} LFQueue;

/*
    入队，一次拷贝
 */
int LFQueue_push(LFQueue *queue, const void *buf, uint64_t size, uint64_t *seq);

/*
    出队，一次拷贝
 */
int LFQueue_pop(LFQueue *queue, void *buf, uint64_t *size, uint64_t *seq);

/*
 * 零拷贝入队方法，分两步走
 */
int LFQueue_get_push_ptr(LFQueue *queue, void **pp, uint32_t *id_ptr, uint64_t size);
uint64_t LFQueue_confirm_push(LFQueue *queue, uint32_t id);

/*
 * 零拷贝出队，分两步走
 */
int LFQueue_get_pop_ptr(LFQueue *queue,
                        void **pp,
                        uint64_t *size,
                        uint32_t *id_ptr,
                        uint64_t *seq);
void LFQueue_confirm_pop(LFQueue *queue, uint32_t id);

/*
    分配共享内存，创建队列
    @param key 该队列的唯一标识，其他进程通过该key来获取队列
    @param data_size 每条数据的最大长度，会向上扩展为64的倍数
    @param count 队列能容纳的最大数据条数，会向上扩展为2的幂
    @return 0表示创建成功，-1表示创建失败
 */
int LFQueue_create(int key, uint32_t user_id, uint64_t data_size,
                   uint32_t count, bool overwrite);

/*
    销毁队列，回收共享内存
    @param key 队列的唯一表示
    @return 0表示成功销毁表示为key的队列，-1表示销毁失败（共享内存不存在等原因导致）
 */
int LFQueue_destroy(int key);

/*
    将队列重置未初始化后的状态
 */
void LFQueue_reset(LFQueue *queue);

/*
    对于一块已创建并初始化成为队列的内存，调用该函数将队列注册到queue中
 */
int LFQueue_init(LFQueue *queue, void *mem, uint32_t user_id);

/*
    分配一个LFQueue结构体并打开队列
 */
LFQueue *LFQueue_open(int key, uint32_t user_id);

/*
    关闭一个队列并回收LFQueue
 */
void LFQueue_close(LFQueue *queue);

/*
    暂停当前队列的所有入队出队及查看操作，因为有可能当前已经有进程在读写了，调用该函数后需要
    等待一段时间后才能视为已暂停
 */
void LFQueue_pause(LFQueue *queue);

/*
    从暂停中恢复
 */
void LFQueue_resume(LFQueue *queue);

/*
    打印当前队列信息，如需要准确数据，先将队列暂停并等待100毫秒左右
 */
void LFQueue_print(LFQueue *queue);

/*
    将当前队列存于文件，函数内部不会暂停，需要外部自行暂停，执行失败并不影响队列的使用。
 */
int LFQueue_dump(LFQueue *queue, const char *filename);

/*
    将存于文件的队列加载到内存中，函数内部不会暂停队列，需要外部自行暂停，执行失败则原队列不可用。
    queue必须是一个已经完成初始化的队列，且其大小于文件中的队列应该相等（大于也可以）
 */
int LFQueue_load(LFQueue *queue, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
