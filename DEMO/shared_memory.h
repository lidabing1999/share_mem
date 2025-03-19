#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>

#define SHM_NAME "/timestamp_queue"
#define QUEUE_SIZE 1024

// 时间戳结构体
typedef struct {
    struct timespec timestamp;
} TimeData;

// 无锁环形队列结构体
typedef struct {
    atomic_size_t head;
    atomic_size_t tail;
    TimeData buffer[QUEUE_SIZE];
} SharedQueue;

// 队列操作函数
static inline bool queue_push(SharedQueue *queue, const TimeData *data) {
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t next_tail = (tail + 1) % QUEUE_SIZE;
    
    if (next_tail == atomic_load_explicit(&queue->head, memory_order_acquire)) {
        return false; // 队列已满
    }
    
    queue->buffer[tail] = *data;
    atomic_store_explicit(&queue->tail, next_tail, memory_order_release);
    return true;
}

static inline bool queue_pop(SharedQueue *queue, TimeData *data) {
    size_t head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    
    if (head == atomic_load_explicit(&queue->tail, memory_order_acquire)) {
        return false; // 队列为空
    }
    
    *data = queue->buffer[head];
    atomic_store_explicit(&queue->head, (head + 1) % QUEUE_SIZE, memory_order_release);
    return true;
}

#endif // SHARED_MEMORY_H