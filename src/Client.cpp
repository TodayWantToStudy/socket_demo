/*
Client.cpp:客户端类实现
*/
#include <iostream>
#include "../include/Client.h"
using namespace std;
//客户端类成员函数
/*
客户端工作流程：
    初始化
  连接服务器
  创建子进程
    子进程                      父进程
向管道写用户输入      从管道读用户输入||处理服务器发送的消息
     重复             处理用户输入      私聊 || 群发
                                重复

*/
Client::Client(){
    //Server.h 中定义 struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;                    //AF_INET 主要是用于互联网地址，而 PF_INET 是协议相关
    serverAddr.sin_port = htons(SERVER_PORT);           //使用htons将服务器端口转化为网格数据
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);  //将点形十进制网络地址转化为32位二进制网络字节序的IPV4地址
    //初始化socket描述符
    sock = 0;
    //初始化进程号
    pid = 0;
    //初始化客户端状态
    isClientwork = true;
    //初始化epoll描述符
    epfd = 0;
}

//启动客户端
void Client::Start(){
    //epoll时间队列
    static struct epoll_event events[2];   
    //创建socket，epoll，pipe，连接服务器
    Connect();
    //调用fork()创建子进程，子进程拷贝一份父进程的内存副本，（并非共享内存空间）
    //如果创建成功：
    //  在子进程中，fork()会返回0，在父进程中会返回1,可以通过返回值判断在哪一个进程中
    //如果创建失败：
    //  返回负值
    pid = fork();       //fork()调用一次可能会返回两次（如果成功）

    //如果创建子进程失败则退出
    if(pid < 0){
        perror("fork error");
        close(sock);
        exit(-1);
    }
    else if(pid == 0){      //通过fork()的返回值为0判断出在子进程中
        //进入子进程执行流程
        //子进程负责写入管道，因此先关闭读端
        close(pipe_fd[0]);
        //输入exit可以退出聊天室
        cout<<"Please input 'exit' to exit the chat room."<<endl;
        cout<<"\\ + ClientID to private chat."<<endl;
        //如果客户端运行正常则不断读取输入发送到服务端
        while(isClientwork){
            //清空结构体
            memset(msg.content, 0, sizeof(msg.content));
            fgets(msg.content, BUF_SIZE, stdin);
            //客户端输入exit，退出
            if(strncasecmp(msg.content, EXIT, strlen(EXIT)) == 0){
                isClientwork = 0;
            }else{   
                //清空发送缓存
                memset(send_buf, 0, BUF_SIZE);
                //结构体转化为字符串
                memcpy(send_buf, &msg, sizeof(msg));
                //子进程将信息写入管道
                if( write(pipe_fd[1], send_buf, sizeof(send_buf)) < 0 ){
                    perror("fork error");
                    exit(-1);
                }
            }
        }
    }else{  //fork()的返回值为1，判断出在父进程。
        //父进程负责读管道数据,因此先关闭写端
        close(pipe_fd[1]);
        //读取循环(epoll wait)
        while(isClientwork){
            //int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
            int epoll_event_count = epoll_wait( epfd, events, 2, -1);   //永久堵塞直至事件触发：服务器端消息、管道消息
            //处理就绪事件
            for(int i = 0; i < epoll_event_count; ++i){
                //清除接受缓存
                memset(recv_buf, 0, sizeof(recv_buf));
                //服务端发来信息
                if(events[i].data.fd == sock){
                    //接受服务端广播信息
                    int ret = recv(sock, recv_buf, BUF_SIZE, 0);
                    //清空结构体
                    memset(&msg, 0, sizeof(msg));
                    //将接收到的消息转换为结构体
                    memcpy(&msg, recv_buf, sizeof(msg));

                    //ret == 0 服务端关闭
                    if(ret == 0){
                        cout<<"Server close connection: "<<sock<<endl;
                        close(sock);
                        isClientwork = 0;
                    }else{
                        cout<<msg.content<<endl;
                    }
                }else{
                    //父进程写入事件发生，父进程处理并发送到服务端
                    //父进程从管道中读取数据
                    int ret = read(events[i].data.fd, recv_buf, BUF_SIZE);
                    //ret == 0 服务器关闭
                    if(ret == 0)
                        isClientwork = 0;
                    else{
                        //将从管道中读取的字符串发送给服务端
                        send(sock, recv_buf, sizeof(recv_buf), 0);
                    }
                }
            }//for
        }//while
    }
    //退出进程
    Close();
}

//创建一个socket连接服务器
void Client::Connect(){
    cout<<"Connect Server: "<<SERVER_IP<<" : "<<SERVER_PORT<<endl;
    //创建socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock<0){
        perror("sock error");
        exit(-1);
    }
    //连接服务器
    if(connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 ){
        perror("connect error");
        exit(-1);
    }
    //父进程创建管道，
    //其中fd[0]用于父进程读取，fd[1]用于子进程写
    if(pipe(pipe_fd) < 0){
        perror("pipe error");
        exit(-1);
    }
    //创建epoll
    epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("epfd error");
        exit(-1);
    }
    //将sock和管道读端描述符都添加到内核事件表中
    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true);
}

//断开连接，清理并关闭文件描述符
void Client::Close(){
    if(pid){
        //关闭父进程的管道和sock
        close(pipe_fd[0]);
        close(sock);
    }
    else{
        //关闭子进程的管道
        close(pipe_fd[1]);
    }

}