#ifndef CHATROOM_CLIENT_H
#define CHATROOM_CLIENT_H

#include <string>
#include "Common.h"

//客户端类实现
class Client{
public:
    Client();
    //连接服务器
    void Connect();
    //断开连接
    void Close();
    //启动客户端
    void Start();

private:
    //当前连接服务器创建的socket
    int sock;
    //当前进程ID
    int pid;
    //epoll_create创建后的返回值
    int epfd;
    //创建管道,其中fd[0]用于父进程读，fd[1]用于子进程写
    int pipe_fd[2];
    //表示客户端是否正常工作
    bool isClientwork;

    //聊天信息
    Msg msg;
    //结构体转换为字符串
    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];
    //用户连接服务器的IP + PORT
    struct sockaddr_in serverAddr;
};
#endif