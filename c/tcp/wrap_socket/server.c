#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "wrap.h"

#define SERV_PORT 8888

int main(void)
{
    int sfd, cfd;
    int len, i;
    char buf[BUFSIZ], clie_IP[BUFSIZ];

    struct sockaddr_in serv_addr, clie_addr;
    socklen_t clie_addr_len;


    /*创建一个socket 指定IPv4协议族 TCP协议*/
    //返回监听文件描述符
    sfd = Socket(AF_INET, SOCK_STREAM, 0);

    /*初始化一个地址结构 man 7 ip 查看对应信息*/
    //"192.168.1.24" --》unsigned int -》 htonl() -》 网络字节序
    bzero(&serv_addr, sizeof(serv_addr));           //将整个结构体清零
    serv_addr.sin_family = AF_INET;                 //选择协议族为IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //监听本地所有IP地址
    serv_addr.sin_port = htons(SERV_PORT);          //绑定端口号           

    /*绑定服务器地址结构*/
    //将ip+port绑定到监听文件描述符
    Bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    /*设定链接上限,注意此处不阻塞*/
    Listen(sfd, 2);            //同一时刻允许向服务器发起链接请求的数量                     

    //端口复用，服务端处理wait_time，可以使用服务端开启不会报端口被占用
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    /**
     * 服务器必须准备好接受外来的连接，socket,bin,linsten这三个函数来完成，称之为被动打开
     * 
     */
    
    printf("wait for client connect ...\n");

    /*获取客户端地址结构大小*/ 
    clie_addr_len = sizeof(clie_addr_len);
    
    /*参数1是sfd; 参2传出参数, 参3传入传入参数, 全部是client端的参数*/
    //等待客户端连接，内核实现三次握手
    cfd = Accept(sfd, (struct sockaddr *)&clie_addr, &clie_addr_len);
    printf("cfd = ----%d\n", cfd);

    printf("client IP: %s  port:%d\n", 
            inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP)), 
            ntohs(clie_addr.sin_port));

    //死循环，不断接收客户端发送过来的数据，除非客户端，断开连接，否则不会退出死循环
    while (1) {
        /*读取客户端发送数据*/
        len = Read(cfd, buf, sizeof(buf));
        Write(STDOUT_FILENO, buf, len);

        /*处理客户端数据*/
        for (i = 0; i < len; i++)
            buf[i] = toupper(buf[i]);

        /*处理完数据回写给客户端*/
        Write(cfd, buf, len); 
    }

    /*关闭链接*/
    Close(sfd);
    //四次挥手
    Close(cfd);

    return 0;
}
