#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <unistd.h>

#include "wrap.h"

#define MAXLINE 8192
#define SERV_PORT 8888

void do_sigchild(int num)
{
    while (waitpid(0, NULL, WNOHANG) > 0)
        ;
}

int main(void)
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len;
    int listenfd, connfd;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];
    int i, n;
    pid_t pid;
    struct sigaction newact;

    /*设置信号量，访问子进程僵死*/
    //子进程响应SIGCHLD
    newact.sa_handler = do_sigchild;
    sigemptyset(&newact.sa_mask);
    newact.sa_flags = 0;
    sigaction(SIGCHLD, &newact, NULL);

    /*创建一个socket 指定IPv4协议族 TCP协议*/
    //返回监听文件描述符
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /*初始化一个地址结构 man 7 ip 查看对应信息*/
    //"192.168.1.24" --》unsigned int -》 htonl() -》 网络字节序

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    /*绑定服务器地址结构*/
    //将ip+port绑定到监听文件描述符
    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    /*设定链接上限,注意此处不阻塞*/
    Listen(listenfd, 20);

    printf("Accepting connections ...\n");

    //死循环，不断接收客户端连接
    while (1)
    {
        cliaddr_len = sizeof(cliaddr);
        /*参数1是sfd; 参2传出参数, 参3传入传入参数, 全部是client端的参数*/
        //等待客户端连接，内核实现三次握手
        connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        pid = fork(); //开进程
        if (pid == 0)
        {
            //开起子进程
            Close(listenfd);
            //死循环，不断接收客户端发送过来的数据，除非客户端，断开连接，否则不会退出死循环

            while (1)
            {
                /**处理客户端接收的数据*/
                n = Read(connfd, buf, MAXLINE);
                if (n == 0)
                {
                    printf("the other side has been closed.\n");
                    break;
                }
                printf("received from %s at PORT %d\n",
                       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                       ntohs(cliaddr.sin_port));

                for (i = 0; i < n; i++)
                    buf[i] = toupper(buf[i]);

                Write(STDOUT_FILENO, buf, n);
                Write(connfd, buf, n);
            }
            Close(connfd);
            return 0;
        }
        else if (pid > 0)
        {
            //父进程
            Close(connfd);
        }
        else
            perr_exit("fork");
    }
    return 0;
}
