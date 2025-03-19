#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include "shared_memory.h"

// 错误处理函数
static void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int shm_fd;
    SharedQueue *queue;
    TimeData data;
    
    // 创建或打开共享内存
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (shm_fd == -1) {
        handle_error("shm_open failed");
    }
    
    // 设置共享内存大小
    if (ftruncate(shm_fd, sizeof(SharedQueue)) == -1) {
        handle_error("ftruncate failed");
    }
    
    // 映射共享内存
    queue = mmap(NULL, sizeof(SharedQueue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (queue == MAP_FAILED) {
        handle_error("mmap failed");
    }
    
    // 获取高精度时间戳
    if (clock_gettime(CLOCK_MONOTONIC, &data.timestamp) == -1) {
        handle_error("clock_gettime failed");
    }
    
    // 将时间戳写入队列
    if (!queue_push(queue, &data)) {
        fprintf(stderr, "Queue is full\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Sent timestamp: %ld.%09ld\n", data.timestamp.tv_sec, data.timestamp.tv_nsec);
    
    // 清理资源
    munmap(queue, sizeof(SharedQueue));
    close(shm_fd);
    
    return 0;
}