
/*
Server.h:服务器端的类声明，定义服务器端需要具备的函数和变量
*/
#ifndef CHATROOM_SERVER_H
#define CHATROOM_SERVER_H
 
#include <string>
#include "Common.h"
 
using namespace std;
 
// 服务端类，用来处理客户端请求
class Server {
 
public:
    // 无参数构造函数
    Server();
 
    // 初始化服务器端设置
    void Init();
 
    // 关闭服务
    void Close();
 
    // 启动服务端
    void Start();
 
private:
    // 广播消息给所有客户端
    int SendBroadcastMessage(int clientfd);
 
    // 服务器端serverAddr信息
    struct sockaddr_in serverAddr;
/*  struct sockaddr_in
    {
        short sin_family;           //Address family一般来说AF_INET（地址族）PF_INET（协议族）
        unsigned short sin_port;    //Port number(必须要采用网络数据格式,普通数字可以用htons()函数转换成网络数据格式的数字)
        struct in_addr sin_addr;    //IP address in network byte order（Internet address）
        unsigned char sin_zero[8];  //Same size as struct sockaddr没有实际意义,只是为了　跟SOCKADDR结构在内存中对齐
    };
*/
    //创建监听的socket
    int listener;
 
    // epoll_create创建后的返回值
    int epfd;
    
    // 客户端列表
    list<int> clients_list;
};

#endif