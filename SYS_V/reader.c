/**
 * SYS V共享内存示例代码 - 读取端
 * 该程序从共享内存读取数据并在完成后清理共享内存
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>

#define SHM_SIZE 1024  // 共享内存大小（字节）
#define SHM_KEY 12345  // 共享内存键值

int main() {
    int shmid;
    char *shm_ptr;
    
    // 获取已存在的共享内存段
    shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存获取成功，ID: %d\n", shmid);
    
    // 将共享内存附加到进程的地址空间
    shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存附加成功\n");
    
    // 从共享内存读取数据
    printf("从共享内存读取的数据: %s\n", shm_ptr);
    
    // 从进程分离共享内存
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存已分离\n");
    
    // 删除共享内存段
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存已删除\n");
    printf("读取端已完成\n");
    
    return 0;
}