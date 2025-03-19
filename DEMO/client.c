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
#include <time.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/memfd-demo.socket"
#define BUFFER_SIZE 256

// 错误处理函数
static void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// 发送文件描述符
static void send_fd(int socket, int fd) {
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(sizeof(int))];
    char data = 'x';
    struct iovec io = { .iov_base = &data, .iov_len = 1 };
    
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    
    *((int *) CMSG_DATA(cmsg)) = fd;
    
    if (sendmsg(socket, &msg, 0) < 0) {
        handle_error("sendmsg failed");
    }
}

int main() {
    int sock_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    struct timeval tv;
    
    // 创建memfd
    int memfd = memfd_create("timestamp", 0);
    if (memfd == -1) {
        handle_error("memfd_create failed");
    }
    
    // 获取当前时间戳（精确到微秒）
    gettimeofday(&tv, NULL);
    snprintf(buffer, BUFFER_SIZE, "%ld.%06ld\n", tv.tv_sec, tv.tv_usec);
    
    // 写入时间戳到共享内存
    if (write(memfd, buffer, strlen(buffer)) == -1) {
        handle_error("write to memfd failed");
    }
    
    // 创建Unix域套接字
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        handle_error("socket creation failed");
    }
    
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // 连接到服务器
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        handle_error("connect failed");
    }
    
    printf("Connected to server. Sending timestamp: %s", buffer);
    
    // 发送文件描述符
    send_fd(sock_fd, memfd);
    
    // 清理资源
    close(memfd);
    close(sock_fd);
    
    return 0;
}