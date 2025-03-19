#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/memfd-demo.socket"
#define BUFFER_SIZE 256

// 错误处理函数
static void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// 接收文件描述符
static int recv_fd(int socket) {
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(int))];
    char data;
    struct iovec io = { .iov_base = &data, .iov_len = 1 };
    
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    
    if (recvmsg(socket, &msg, 0) < 0) {
        handle_error("recvmsg failed");
    }
    
    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) {
        handle_error("no file descriptor received");
    }
    
    return *((int *) CMSG_DATA(cmsg));
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    
    // 创建Unix域套接字
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        handle_error("socket creation failed");
    }
    
    // 删除可能存在的旧socket文件
    if (unlink(SOCKET_PATH) == -1 && errno != ENOENT) {
        handle_error("unlink failed");
    }
    
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        handle_error("bind failed");
    }
    
    if (listen(server_fd, 5) == -1) {
        handle_error("listen failed");
    }
    
    printf("Server is listening on %s\n", SOCKET_PATH);
    
    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("New client connected\n");
        
        // 接收文件描述符
        int memfd = recv_fd(client_fd);
        
        // 将文件指针重置到开始位置
        // if (lseek(memfd, 0, SEEK_SET) == -1) {
        //     perror("lseek failed");
        //     close(memfd);
        //     close(client_fd);
        //     continue;
        // }

        // 获取文件大小
        off_t file_size = lseek(memfd, 0, SEEK_END);
        if (file_size == -1) {
            perror("lseek failed");
            close(memfd);
            close(client_fd);
            continue;
        }

        // 重新将文件指针设置到开始位置
        if (lseek(memfd, 0, SEEK_SET) == -1) {
            perror("lseek failed");
            close(memfd);
            close(client_fd);
            continue;
        }

        // 读取共享内存内容
        ssize_t bytes_read = read(memfd, buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Received timestamp: %s", buffer);
            
            // 解析接收到的时间戳
            struct timeval client_tv;
            if (sscanf(buffer, "%ld.%ld", &client_tv.tv_sec, &client_tv.tv_usec) == 2) {
                // 获取当前时间戳
                struct timeval current_tv;
                gettimeofday(&current_tv, NULL);
                
                // 计算时间差（以微秒为单位）
                long long time_diff = (current_tv.tv_sec - client_tv.tv_sec) * 1000000LL +
                                    (current_tv.tv_usec - client_tv.tv_usec);
                
                printf("Time difference: %lld microseconds\n", time_diff);
            } else {
                printf("Error: Invalid timestamp format\n");
            }
        } else {
            perror("read failed");
        }
        
        // 清理资源
        close(memfd);
        close(client_fd);
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}