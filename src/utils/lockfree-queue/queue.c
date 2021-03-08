// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/utils/lockfree-queue/queue.h"

#include <unistd.h>
#include <sys/shm.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int LFQueue_push(LFQueue *queue, const void *buf, uint64_t size, uint64_t *seq)
{
        uint32_t id;
        uint64_t push_seq;
        LFNode *n;
        LFHeader *header = queue->header;

        if (size > header->node_data_size)
                return -1;

        if (header->pause)
                return -3;

        id = LFRing_pop(queue->resc_ring, NULL);
        if (id == LFRING_INVALID_ID) {
                if (header->overwrite) {
                        id = LFRing_pop(queue->node_ring, NULL);
                        if (id == LFRING_INVALID_ID)
                                return -1;
                } else {
                        return -2;
                }
        }

        n = (LFNode *)(queue->nodes + header->node_total_size * id);
        n->size = size;
        memcpy(n->data, buf, size);
        
        push_seq = LFRing_push(queue->node_ring, id);
        if (seq)
                *seq = push_seq;
        
        return 0;
}

int LFQueue_pop(LFQueue *queue, void *buf, uint64_t *size, uint64_t *seq)
{
        uint32_t id;
        uint64_t pop_seq = -1L;
        LFNode *n;
        LFHeader *header = queue->header;

        do {
                if (header->pause)
                        return -3;
                id = LFRing_pop(queue->node_ring, &pop_seq);
        } while (id == LFRING_INVALID_ID);

        n = (LFNode *)(queue->nodes + header->node_total_size * id);
        memcpy(buf, n->data, n->size);
        
        if (size)
                *size = n->size;
        
        if (seq)
                *seq = pop_seq;

        LFRing_push(queue->resc_ring, id);
        return 0;
}

int LFQueue_get_push_ptr(LFQueue *queue, void **pp, uint32_t *id_ptr, uint64_t size)
{
        uint32_t id;
        uint64_t push_seq;
        LFNode *n;
        LFHeader *header = queue->header;

        if (size > header->node_data_size)
                return -1;

        if (header->pause)
                return -3;

        id = LFRing_pop(queue->resc_ring, NULL);
        if (id == LFRING_INVALID_ID) {
                if (header->overwrite) {
                        id = LFRing_pop(queue->node_ring, NULL);
                        if (id == LFRING_INVALID_ID)
                                return -1;
                } else {
                        return -2;
                }
        }

        n = (LFNode *)(queue->nodes + header->node_total_size * id);
        n->size = size;

        *pp = n->data;
        *id_ptr = id;
        return 0;
}

uint64_t LFQueue_confirm_push(LFQueue *queue, uint32_t id)
{
        return LFRing_push(queue->node_ring, id);
}

int LFQueue_get_pop_ptr(LFQueue *queue,
                        void **pp,
                        uint64_t *size,
                        uint32_t *id_ptr,
                        uint64_t *seq)
{
        uint32_t id;
        uint64_t pop_seq = -1L;
        LFNode *n;
        LFHeader *header = queue->header;

        do {
                if (header->pause)
                        return -3;
                id = LFRing_pop(queue->node_ring, &pop_seq);
        } while (id == LFRING_INVALID_ID);

        n = (LFNode *)(queue->nodes + header->node_total_size * id);
        
        if (pp)
                *pp = n->data;

        if (size)
                *size = n->size;

        *id_ptr = id;
        
        if (seq)
                *seq = pop_seq;

        
        return 0;
}

void LFQueue_confirm_pop(LFQueue *queue, uint32_t id)
{
        LFRing_push(queue->resc_ring, id);
}

int LFQueue_create(int key, uint32_t user_id, uint64_t data_size,
                   uint32_t count, bool overwrite)
{
        int shmid;
        uint64_t queue_size = 0;
        uint64_t ring_size, node_size;
        LFHeader *header;
        char *m;

        count = upper_power_of_two(count);
        data_size = (data_size + 63) & ~63;
        ring_size = LFRing_size(count);
        node_size = data_size + sizeof(LFNode);

        queue_size += sizeof(LFHeader);
        queue_size += ring_size * 2;
        queue_size += node_size * count;

        if ((shmid = shmget(key, queue_size, IPC_CREAT | IPC_EXCL | 0666)) < 0)
                return -1;

        if ((m = (char *)shmat(shmid, NULL, 0)) == NULL)
                return -1;

        header = (LFHeader *)m;
        header->magic = QUEUE_MAGIC;
        header->node_count = count;
        header->node_data_size = data_size;
        header->node_total_size = node_size;
        header->overwrite = overwrite;
        header->user_id = user_id;
        header->key = key;

        m += sizeof(LFHeader);
        LFRing_init((LFRing *)m, count, count);
        
        m += ring_size;
        LFRing_init((LFRing *)m, count, 0);

        m += ring_size;
        memset(m, 0, node_size * count);

        return 0;
}

void LFQueue_reset(LFQueue *queue)
{
        uint32_t count = queue->header->node_count;
        uint64_t data_size = queue->header->node_data_size;
        uint64_t ring_size, node_size;
        char *m;

        ring_size = sizeof(LFRing) + sizeof(LFRingNode) * count;
        node_size = data_size + sizeof(LFNode);
        m = (char *)queue->header;

        m += sizeof(LFHeader);
        LFRing_init((LFRing *)m, count, count);

        m += ring_size;
        LFRing_init((LFRing *)m, count, 0);

        m += ring_size;
        memset(m, 0, node_size * count);
}

int LFQueue_init(LFQueue *queue, void *mem, uint32_t user_id)
{
        char *m = (char *)mem;
        uint64_t ring_total_size;

        queue->header = (LFHeader *)m;
        if (queue->header->magic != QUEUE_MAGIC)
                return -1;
        if (queue->header->user_id != user_id)
                return -1;
        ring_total_size = sizeof(LFRing) +
                          queue->header->node_count * sizeof(LFRingNode);

        m += sizeof(LFHeader);
        queue->resc_ring = (LFRing *)m;

        m += ring_total_size;
        queue->node_ring = (LFRing *)m;

        m += ring_total_size;
        queue->nodes = m;

        return 0;
}

int LFQueue_destroy(int key)
{
        int shmid = shmget(key, 0, 0);
        if (shmid < 0)
                return -1;
        return shmctl(shmid, IPC_RMID, NULL);
}

LFQueue *LFQueue_open(int key, uint32_t user_id)
{
        int shmid;
        char *m;
        LFQueue *queue;

        if ((shmid = shmget(key, 0, 0)) < 0)
                return NULL;

        if ((m = shmat(shmid, NULL, 0)) == NULL)
                return NULL;

        if ((queue = malloc(sizeof(LFQueue))) == NULL) {
                shmdt(m);
                return NULL;
        }

        if (LFQueue_init(queue, m, user_id) != 0) {
                shmdt(m);
                free(queue);
                return NULL;
        }

        return queue;
}

void LFQueue_close(LFQueue *queue)
{
        if (queue) {
                if (queue->header->key >= 0)
                        shmdt(queue->header);
                free(queue);
        }
}

void LFQueue_pause(LFQueue *queue)
{
        queue->header->pause = true;
}

void LFQueue_resume(LFQueue *queue)
{
        queue->header->pause = false;
}

void LFQueue_print(LFQueue *queue)
{
        LFHeader *header = queue->header;
        LFRing *ring;

        printf("LFQueue\n");

        printf("\tLFHeader:\n");
        printf("\t\tMagic Number:\t%lu\n", header->magic);
        printf("\t\tNode Data Size:\t%lu\n", header->node_data_size);

        ring = queue->resc_ring;
        printf("\tResource LFRing:\n");
        printf("\t\tNode Count:\t%lu\n", ring->node_count);
        printf("\t\tHead Seq:\t%lu\n", ring->head_seq);
        printf("\t\tTail Seq:\t%lu\n", ring->tail_seq);
        printf("\tAvailable Nodes:\t%lu\n", ring->head_seq - ring->tail_seq);

        ring = queue->node_ring;
        printf("\tNode LFRing:\n");
        printf("\t\tNode Count:\t%lu\n", ring->node_count);
        printf("\t\tHead Seq:\t%lu\n", ring->head_seq);
        printf("\t\tTail Seq:\t%lu\n", ring->tail_seq);
        printf("\tAvailable Nodes:\t%lu\n", ring->head_seq - ring->tail_seq);
}

int LFQueue_dump(LFQueue *queue, const char *filename)
{
        uint64_t size;
        LFHeader *header = queue->header;
        FILE *fp;

        size = sizeof(LFHeader) + 2 * LFRing_size(header->node_count) +
               header->node_total_size * header->node_count;

        if ((fp = fopen(filename, "wb")) == NULL)
                return -1;

        if (fwrite(header, 1, size, fp) != size) {
                fclose(fp);
                return -1;
        }

        fclose(fp);
        return 0;
}

int LFQueue_load(LFQueue *queue, const char *filename)
{
        uint64_t size;
        LFHeader *header = queue->header;
        FILE *fp;

        if ((fp = fopen(filename, "rb")) == NULL)
                return -1;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (fread(header, 1, size, fp) != size) {
                fclose(fp);
                return -1;
        }

        fclose(fp);
        return 0;
}
