/**
 * SYS V共享内存示例代码 - 写入端
 * 该程序创建共享内存并写入数据
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
    char buffer[SHM_SIZE];
    
    // 创建共享内存段
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存创建成功，ID: %d\n", shmid);
    
    // 将共享内存附加到进程的地址空间
    shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存附加成功\n");
    
    // 向共享内存写入数据
    printf("请输入要写入共享内存的数据: ");
    fgets(buffer, SHM_SIZE, stdin);
    
    // 复制数据到共享内存
    strncpy(shm_ptr, buffer, SHM_SIZE);
    
    printf("数据已写入共享内存: %s\n", shm_ptr);
    
    // 从进程分离共享内存
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存已分离\n");
    printf("写入端已完成，请运行读取端程序读取数据\n");
    
    return 0;
}