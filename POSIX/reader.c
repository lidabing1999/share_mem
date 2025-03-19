/**
 * POSIX共享内存示例代码 - 读取端
 * 该程序从共享内存中读取数据并在完成后清理共享内存
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
    
    // 打开共享内存对象
    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存打开成功，FD: %d\n", shm_fd);
    
    // 将共享内存映射到进程的地址空间
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存映射成功\n");
    
    // 读取并显示共享内存中的内容
    printf("从共享内存中读取的内容: %s\n", (char *)shm_ptr);
    
    // 从进程解除共享内存映射
    if (munmap(shm_ptr, SHM_SIZE) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    
    // 关闭共享内存文件描述符
    close(shm_fd);
    
    // 删除共享内存对象
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed");
        exit(EXIT_FAILURE);
    }
    
    printf("共享内存已解除映射、关闭并删除\n");
    
    return 0;
}