#!/bin/bash

# 设置编译器和编译选项
CC=gcc
CFLAGS="-Wall -O2"

# 编译server程序
echo "编译 server.c..."
$CC $CFLAGS server.c -o server
if [ $? -eq 0 ]; then
    echo "server 编译成功"
else
    echo "server 编译失败"
    exit 1
fi

# 编译client程序
echo "编译 client.c..."
$CC $CFLAGS client.c -o client
if [ $? -eq 0 ]; then
    echo "client 编译成功"
else
    echo "client 编译失败"
    exit 1
fi

echo "所有程序编译完成"