/*
Server.cpp:客户端类实现
*/
#include <iostream>
#include "../include/Server.h"
using namespace std;
//服务端类成员函数

Server::Server(){
    //Server.h 中定义 struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;                    //AF_INET 主要是用于互联网地址，而 PF_INET 是协议相关
    serverAddr.sin_port = htons(SERVER_PORT);           //使用htons将服务器端口转化为网格数据
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);  //将点形十进制网络地址转化为32位二进制网络字节序的IPV4地址
    listener = 0;   //初始化套接字
    epfd = 0;       //初始化epoll文件描述符
}

void Server::Init(){
    cout<<"Init Server..."<<endl;
    //创建监听socket
    listener = socket(PF_INET, SOCK_STREAM, 0);
    //socket()函数根据给定的一套地址族（协议族），数据类型和协议分配一个套接口的描述字
    if(listener<0){
        //socket()分配失败，则报错退出
        perror("listener");
        exit(-1);
    }
    //绑定地址，将套接口描述字与本地端口绑定起来，
    //socket()创建套接口后，它便存在于一个名字空间（地址族）中，但并未赋名。
    //bind()函数通过给一个未命名套接口分配一个本地名字来为套接口建立本地捆绑（主机地址/端口号）。
    if(bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 ){
        perror("bind error");
        exit(-1);
    }
    //监听
    //创建套接口，并为申请进入的连接建立一个后备日志
    int ret = listen(listener, 5);
    if(ret < 0){
        perror("listener error");
        exit(-1);
    }
    cout<<"Start to listen: "<<SERVER_IP<<endl;
    //在内核中创建事件表 epfd是一个句柄
    //创建一个epoll的句柄，告诉内核这个监听的数目
    epfd = epoll_create(EPOLL_SIZE);
    if(epfd<0){
        perror("epfd error");
        exit(-1);
    }
    //调用自定义的addfd函数(Common.h)
    //将代表套接字的文件描述符listener加入到epfd代表的epoll中
    //在内核的监听列表中注册套接字的文件描述符，true表示ET边缘触发
    addfd(epfd, listener, true);   
    
    //初始化完成
}

void Server::Close(){
    //关闭socket
    close(listener);
    //关闭epoll监听
    close(epfd);
}

//启动服务器
void Server::Start(){
    //epoll 事件队列
    static struct epoll_event events[EPOLL_SIZE];

    //初始化服务器
    Init();

    //主循环
    while(1){
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);

        if(epoll_events_count<0){
            perror("epoll failure");
            break;
        }

        cout<<"epoll_events_count = \n"<<epoll_events_count<<endl;

        //处理这些就绪事件
        //在服务器类构造函数中，已经内核已经分配了套接口（描述字形式）给程序，程序将套接口与本地端口绑定
        //这套接口（本地端口）将被用于新用户注册。
        //服务器刚启动时，epoll只有用于注册的套接口描述字。
        //随着客户端连接进服务器，服务器将不断将通过accept()分配新的套接口给不同的客户端。
        //服务器也不断地将新用户的套接口描述符注册到epoll中，以后信息传输将通过新的描述符，区别于注册套接口描述符。
        for(int i=0;i<epoll_events_count;++i){
            int sockfd = events[i].data.fd;
            //如果描述符是注册套接口描述符，则从等待连接队列中抽取第一个连接。
            //获取这个连接的实体地址，为这个连接分配专用套接字描述符，并将这个描述符添加到epoll。
            if(sockfd == listener){
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(listener, (struct sockaddr* )&client_address, &client_addrLength );

                cout<<"client connection from: "
                    << inet_ntoa(client_address.sin_addr) <<":"
                    << ntohs(client_address.sin_port) <<", clientfd = "
                    <<  clientfd <<endl;
                addfd(epfd, clientfd, true);

                //服务器端用list保存用户连接
                clients_list.push_back(clientfd);
                cout<<"Add new clientfd = "<<clientfd<<" to epoll"<<endl;
                cout<<"Now there are "<<clients_list.size()<<" clients in the chat room."<<endl;

                //服务端发布欢迎信息
                cout<<"welcome message"<<endl;
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);
                sprintf(message, SERVER_WELCOME, clientfd);
                int ret = send(clientfd,message, BUF_SIZE, 0);
                if(ret<0){
                    perror("send error");
                    Close();
                    exit(-1);
                }
            }
            //如果这个描述符不是注册套接字描述符，那必然是用于用户通信的专用描述符
            //则调用SendBroadcastMessage()函数广播消息，参数为用于某用户通信的套接字描述符
            else{
                int ret = SendBroadcastMessage(sockfd);
                if(ret<0){
                    perror("error");
                    Close();
                    exit(-1);
                }
            }
        }
    }
    Close();
}

//发送信息给客户端（群发，私聊）
//参数是信息来源的套接字描述符
//返回成功发送的消息长度
int Server::SendBroadcastMessage(int clientfd){
    //buf[BUF_SIZE]     通过套接字接受到新消息（非结构体）
    //message[BUF_SIZE] 需要发送的格式化的消息
    char recv_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    Msg msg;    //即将要广播的消息（结构体形）
    bzero(recv_buf, BUF_SIZE);//将字符串前n位置0
    //接受新消息，打印消息来源
    cout<<"read from client(clientID = "<<clientfd<<")"<<endl;
    //通过套接字描述符clientfd，接受消息，存放到recv_buf,len为接收到的消息长度
    int len = recv(clientfd, recv_buf, BUF_SIZE, 0);
    //清空结构体，把接收到的字符串转换为结构体
    memset(&msg, 0, sizeof(msg));
    memcpy(&msg, recv_buf, sizeof(msg));
    //填充消息msg
    msg.fromID = clientfd;
    //判断接受到的信息是私聊还是群聊
    if(msg.content[0]=='\\'&&isdigit(msg.content[1])){
        msg.type=1;
        msg.toID=msg.content[1]-'0';
        memcpy(msg.content, msg.content+2, sizeof(msg.content));
    }
    else
        msg.type=0;
    //如果客户端关闭了连接
    if(len==0){
        close(clientfd);
        //在客户端列表中删除该客户端
        clients_list.remove(clientfd);
        cout<<"ClientID = "<<clientfd<<" closed."<<endl
            <<"Now there are "<<clients_list.size()<<" clients in the chat room."<<endl;
    }
    else{   //发送广播消息给所有客户端
        //判断是否聊天室还有其他用户端
        if(clients_list.size()==1){
            //发送提示消息
            memcpy(&msg.content, CAUTION, sizeof(msg.content));
            bzero(send_buf, BUF_SIZE);
            memcpy(send_buf, &msg, sizeof(msg));
            send(clientfd, send_buf, sizeof(send_buf), 0);
            return len;
        }
        char format_message[BUF_SIZE];
        //发送群聊消息
        if(msg.type==0){
            //sprintf:将格式化的数据写入某个字符串
            //格式化发送的消息内容 #define SERVER_MESSAGE "ClientID %d say >> %s"
            sprintf(format_message, SERVER_MESSAGE, clientfd, msg.content);
            //将消息填充到结构体msg的成员content中
            memcpy(msg.content, format_message, BUF_SIZE);
            //遍历客户端列表依次发送消息， 需要判断不要给来源客户端发送
            for(auto it=clients_list.begin();it!=clients_list.end();++it){
                if(*it!=clientfd){
                    //把发送的结构体转换为字符串
                    bzero(send_buf, BUF_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if( send(*it, send_buf, sizeof(send_buf),0) < 0 ){
                        return -1;
                    }
                }
            }
        }
        //发送私聊消息
        if(msg.type==1){
            bool private_offline = true;
            sprintf(format_message, SERVER_PRIVATE_MESSAGE, clientfd, msg.content);
            memcpy(msg.content, format_message, BUF_SIZE);
            //遍历客户端列表发送消息，需要判断给目标用户发
            for(auto it=clients_list.begin();it!=clients_list.end();++it){
                if(*it==msg.toID){
                    private_offline = false;
                    //把发送的结构体转换为字符串
                    bzero(send_buf, BUF_SIZE);
                    memcpy(send_buf, &msg, sizeof(msg));
                    if( send(*it, send_buf, sizeof(send_buf),0) < 0 ){
                        return -1;
                    }
                }
            }
            //如果私聊对象不在线
            if(private_offline){
                sprintf(format_message, SERVER_PRIVATE_ERROR_MESSAGE, msg.toID);
                memcpy(msg.content, format_message, BUF_SIZE);
                bzero(send_buf, BUF_SIZE);
                memcpy(send_buf, &msg, sizeof(msg));
                if(send(msg.fromID, send_buf, sizeof(send_buf),0) < 0){
                    return -1;
                }
            }
        }
    }
    return len;
}
