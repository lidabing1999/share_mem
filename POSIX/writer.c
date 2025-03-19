/**
 * POSIX共享内存示例代码 - 写入端
 * 该程序创建共享内存并写入预设数据
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define SHM_SIZE 1024  // 共享内存大小（字节）
#define SHM_NAME "my_posix_shm"  // 共享内存名称

int main() {
    int shm_fd;
    void *shm_ptr;
    const char *message = "这是预设的共享内存内容，使用POSIX API实现。这段内容将被写入共享内存并被读取端读取。";
    
    // 创建共享内存对象
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存创建成功，FD: %d\n", shm_fd);
    
    // 设置共享内存大小
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }
    
    // 将共享内存映射到进程的地址空间
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存映射成功\n");
    
    // 将预设内容写入共享内存
    strncpy((char *)shm_ptr, message, SHM_SIZE);
    
    printf("预设数据已写入共享内存: %s\n", (char *)shm_ptr);
    
    // 从进程解除共享内存映射
    if (munmap(shm_ptr, SHM_SIZE) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    
    // 关闭共享内存文件描述符
    close(shm_fd);
    
    printf("共享内存已解除映射并关闭\n");
    printf("写入端已完成，请运行读取端程序读取数据\n");
    
    return 0;
}