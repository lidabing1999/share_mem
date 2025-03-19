#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <fcntl.h>

// 错误处理函数
static void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

#define SOCKET_PATH "/tmp/fd-pass.socket"
#define SIZE 0x400000
#define MESSAGE "Hello from shared memory!"

// 发送文件描述符
static void send_fd(int socket, int *fds, int n)
{
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n * sizeof(int))], data;
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = &data, .iov_len = 1 };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(n * sizeof(int));

    memcpy((int *) CMSG_DATA(cmsg), fds, n * sizeof(int));

    if (sendmsg(socket, &msg, 0) < 0)
        handle_error("Failed to send message");
}

int main() {
    int sfd = -1, fds[2] = {-1, -1};
    struct sockaddr_un addr;
    void *ptr0 = NULL, *ptr1 = NULL;

    // 创建Unix域套接字
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("Failed to create socket");
        goto cleanup;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // 创建两个共享内存区域
    fds[0] = memfd_create("shma", 0);
    if (fds[0] < 0) {
        perror("Failed to create memfd 1");
        goto cleanup;
    }

    fds[1] = memfd_create("shmb", 0);
    if (fds[1] < 0) {
        perror("Failed to create memfd 2");
        goto cleanup;
    }

    // 设置共享内存大小
    if (ftruncate(fds[0], SIZE) == -1) {
        perror("Failed to set size for memfd 1");
        goto cleanup;
    }
    if (ftruncate(fds[1], SIZE) == -1) {
        perror("Failed to set size for memfd 2");
        goto cleanup;
    }

    // 映射共享内存
    ptr0 = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fds[0], 0);
    if (ptr0 == MAP_FAILED) {
        perror("Failed to mmap region 1");
        goto cleanup;
    }

    ptr1 = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fds[1], 0);
    if (ptr1 == MAP_FAILED) {
        perror("Failed to mmap region 2");
        goto cleanup;
    }

    // 写入数据
    strcpy(ptr0, MESSAGE " (Region 1)");
    strcpy(ptr1, MESSAGE " (Region 2)");

    // 连接到socket
    if (connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Failed to connect to socket");
        goto cleanup;
    }

    // 发送文件描述符
    send_fd(sfd, fds, 2);
    printf("Sent file descriptors to child process\n");

    // 等待一会儿让子进程读取数据
    //sleep(1);

cleanup:
    if (ptr0 != NULL && ptr0 != MAP_FAILED)
        munmap(ptr0, SIZE);
    if (ptr1 != NULL && ptr1 != MAP_FAILED)
        munmap(ptr1, SIZE);
    if (fds[0] >= 0)
        close(fds[0]);
    if (fds[1] >= 0)
        close(fds[1]);
    if (sfd >= 0)
        close(sfd);

    return (ptr0 != NULL && ptr1 != NULL) ? 0 : 1;
}