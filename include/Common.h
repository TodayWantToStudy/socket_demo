/*
Common.h：公共头文件，包括所有需要的宏以及socket网络编程头文件，以及消息结构体（用来表示消息类别等）
*/
#ifndef CHATROOM_COMMON_H
#define CHATROOM_COMMON_H

#include <iostream>
#include <list>
#include <sys/types.h>      //linxu基本系统数据类型
#include <sys/socket.h>     //对传输层TCP/IP协议的抽象接口，有了抽象接口我们可以不用知道TCP/IP具体如何传输
#include <netinet/in.h>     //usr/include/netinet/in.h 声明常用协议、端口，宏定义常用的地址位操作，声明有关地址的机构体
#include <arpa/inet.h>      ///usr/include/arpa/inet.h 声明常用的地址转换，如点分十进制转换为整形二进制
#include <sys/epoll.h>      //linux用于处理大批量文件描述符的poll，能显著提升程序在大量并发连接中只有少量活跃的系统CPU利用率
#include <fcntl.h>          //unix标准中通用的头文件，提供用于处理文件描述符的函数
#include <errno.h>          //C语言标准库的头文件，定义了通过错误码来回报错误资讯的宏
#include <unistd.h>         //提供对 POSIX 操作系统 API 的访问功能的头文件,如fork、pipe以及各种I/O术语
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 默认服务器端IP地址
#define SERVER_IP "172.22.190.216"//"127.0.0.1"
 
// 服务器端口号
#define SERVER_PORT 8888
 
// int epoll_create(int size)中的size
// 为epoll支持的最大句柄数
#define EPOLL_SIZE 5000
 
// 缓冲区大小65535
#define BUF_SIZE 0xFFFF
    

// 新用户登录后的欢迎信息
#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"

// 其他用户收到消息的前缀
#define SERVER_MESSAGE "ClientID %d say >> %s"
#define SERVER_PRIVATE_MESSAGE "Client %d say to you privately >> %s"
#define SERVER_PRIVATE_ERROR_MESSAGE "Client %d is not in the chat room yet~"
// 退出系统
#define EXIT "EXIT"
 
// 提醒你是聊天室中唯一的客户
#define CAUTION "There is only one int the char room!"
 
 
// 注册新的fd到epollfd中  
// fd:file description 文件描述符 epollfd: 创建epoll返回的文件描述符
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
// ET模式:边缘触发模式。当某个fd从为就绪变为就绪时，内核通过epoll告诉程序，
// 然后内核会默认程序已经知道fd已经就绪，并且不再通知，直至该fd被再次变为未就绪。
// LT模式:水平触发模式。内核会不断通知程序某个fd已经就绪，无论程序是否对某个fd操作。

// 定义一个函数将文件描述符fd添加到文件描述符epollfd代表的epoll服务（又称内核事件表）中供客户端和服务端两个类使用。
static void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;      
    //用于表示需要监听的内容
    /*
    struct epoll_event {
        __uint32_t events; 表示需要监听的事件
        epoll_data_t data; 联合体，可用于传参，一般将文件描述符传入fd
    };
    */
    ev.data.fd = fd;        //将文件描述符fd存入data中
    ev.events = EPOLLIN;    //需要监听文件描述符是否可读
    if( enable_et )         //如果开启ET边缘触发模式，则采用边缘触发
        ev.events = EPOLLIN | EPOLLET;         //可读时间按位与边缘触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);//将文件描述符fd添加到epollfd代表的epoll中，并通过ev描述监听内容
    // epoll_ctl用于添加或删除监听文件描述符，通过参数2实现

    // 设置socket为非阻塞模式
    // 套接字立刻返回，不管I/O是否完成，该函数所在的线程会继续运行
    // eg. 在recv(fd...)时，该函数立刻返回，在返回时，内核数据还没准备好会返回WSAEWOULDBLOCK错误代码
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);//fcntl函数可以针对文件描述符进行控制
    // 将文件描述符状态设置为：（获取close-on-exec旗帜，表示调用exec()相关函数文件不会关闭，
    // 然后与O_NONBLOCK按位与，表示将文件描述符的状态设置为非阻塞模式
    printf("fd added to epoll!\n\n");
}
 
//定义信息结构，在服务端和客户端之间传送
struct Msg
{
    int type;       //用于标记广播或者私聊
    int fromID;     //来自哪一个文件描述符
    int toID;       //发到哪一个文件描述符
    char content[BUF_SIZE];//传送的内容
 
};

#endif