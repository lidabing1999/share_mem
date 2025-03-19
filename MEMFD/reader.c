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

// 错误处理函数
static void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

#define SOCKET_PATH "/tmp/fd-pass.socket"
#define SIZE 0x400000

// 接收文件描述符
static int * recv_fd(int socket, int n)
{
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n * sizeof(int))], data;
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = &data, .iov_len = 1 };

    int *fds = malloc(n * sizeof(int));
    if (!fds) {
        perror("malloc failed");
        return NULL;
    }

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(socket, &msg, 0) < 0) {
        perror("recvmsg");
        free(fds);
        return NULL;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) {
        perror("CMSG_FIRSTHDR failed");
        free(fds);
        return NULL;
    }

    memcpy(fds, CMSG_DATA(cmsg), n * sizeof(int));
    return fds;
}

int main() {
    int sfd = -1, cfd = -1;
    int *fds = NULL;
    struct sockaddr_un addr;
    char buffer[256];
    ssize_t nbytes;
    int ret = 1;

    // 创建Unix域套接字
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("Failed to create socket");
        goto cleanup;
    }

    // 删除可能存在的旧socket文件
    if (unlink(SOCKET_PATH) == -1 && errno != ENOENT) {
        perror("Removing socket file failed");
        goto cleanup;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // 绑定socket
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Failed to bind to socket");
        goto cleanup;
    }

    // 监听连接
    if (listen(sfd, 5) == -1) {
        perror("Failed to listen on socket");
        goto cleanup;
    }

    // 接受连接
    cfd = accept(sfd, NULL, NULL);
    if (cfd == -1) {
        perror("Failed to accept incoming connection");
        goto cleanup;
    }

    // 接收文件描述符
    fds = recv_fd(cfd, 2);
    if (!fds) {
        fprintf(stderr, "Failed to receive file descriptors\n");
        goto cleanup;
    }

    // 读取并显示共享内存内容
    for (int i = 0; i < 2; ++i) {
        printf("Reading from passed fd %d\n", fds[i]);
        while ((nbytes = read(fds[i], buffer, sizeof(buffer))) > 0) {
            if (write(STDOUT_FILENO, buffer, nbytes) != nbytes) {
                perror("Failed to write to stdout");
                goto cleanup;
            }
        }
        if (nbytes < 0) {
            perror("Failed to read from shared memory");
            goto cleanup;
        }
    }

    ret = 0;

cleanup:
    if (fds) {
        free(fds);
    }
    if (cfd >= 0) {
        if (close(cfd) == -1) {
            perror("Failed to close client socket");
        }
    }
    if (sfd >= 0) {
        if (close(sfd) == -1) {
            perror("Failed to close server socket");
        }
    }
    unlink(SOCKET_PATH);

    return ret;
}