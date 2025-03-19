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
    struct timespec current_ts;
    
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
    
    printf("Server is waiting for timestamps...\n");
    
    while (1) {
        // 从队列中读取时间戳
        if (queue_pop(queue, &data)) {
            // 获取当前时间戳
            if (clock_gettime(CLOCK_MONOTONIC, &current_ts) == -1) {
                handle_error("clock_gettime failed");
            }
            
            // 计算时间差（以纳秒为单位）
            long long time_diff = (current_ts.tv_sec - data.timestamp.tv_sec) * 1000000000LL +
                                (current_ts.tv_nsec - data.timestamp.tv_nsec);
            
            printf("Received timestamp: %ld.%09ld\n", data.timestamp.tv_sec, data.timestamp.tv_nsec);
            printf("Time difference: %lld nanoseconds\n", time_diff);
        }
        
        // 短暂休眠以避免过度消耗CPU
        //usleep(1000);
    }
    
    // 清理资源（实际上永远不会执行到这里）
    munmap(queue, sizeof(SharedQueue));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    
    return 0;
}