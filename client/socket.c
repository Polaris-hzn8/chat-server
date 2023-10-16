/*************************************************************************
	> File Name: common.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Thu 30 Mar 2023 04:23:36 PM CST
 ************************************************************************/

#include "head.h"
#include "include/socket.h"

// 提供给服务端使用
int socket_create(int port) {
	int sockfd;
	// 1.调用socket函数创建套接字
	// 协议族是 AF_INET（IPv4）
	// 套接字类型是 SOCK_STREAM，表示TCP套接字
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	// 2.绑定套接字与结构体信息
	struct sockaddr_in server;
	server.sin_family = AF_INET;						//ipv4
	server.sin_port = htons(port);						//使用 htons 函数将端口号从主机字节序转换为网络字节序（大端序） host to net short 
	server.sin_addr.s_addr = htonl(INADDR_ANY);			//使用 htonl 函数将 INADDR_ANY 转换为网络字节序 INADDR_ANY监听任何一个地址 0.0.0.0 host to net long
	// 常用于快速重启服务器
	// 调用setsockopt函数，启用SO_REUSEADDR选项，以便可以立即重用绑定地址
	int reuse = 1; setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int));	//服务端地址重用
	if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) return -1;

	// 3.将主动套接字转为被动
	// 使用listen函数将套接字转换为被动套接字，以便可以接受来自客户端的连接请求
	if (listen(sockfd, 20) < 0) return -1;	// 20: 正在连接的连接序列最多限制有多少个

	// 4.返回套接字描述符
	return sockfd;
}

// 提供给客户端使用
int socket_connect(const char *ip, int port) {
	int sockfd;
	// 1.调用socket函数创建套接字
	// 协议族是 AF_INET（IPv4）
	// 套接字类型是 SOCK_STREAM，表示TCP套接字
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	// 2.定义结构体用于绑定端口号、ip地址（存放服务端的具体信息）
	struct sockaddr_in server;
	server.sin_family = AF_INET;						//ipv4
	server.sin_port = htons(port);						//端口号 使用 htons 函数将端口号从主机字节序转换为网络字节序（大端序） host to net short 
	server.sin_addr.s_addr = inet_addr(ip);				//ip地址 inet_addr函数将服务器的IP地址从点分十进制表示转换为网络字节序的二进制形式

	// 3.调用connect函数尝试连接到服务器
	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) return -1;

	// 4.返回套接字描述符
	return sockfd;
}

int make_nonblock(int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFD)) < 0) return -1;
	flags |= O_NONBLOCK;//为GF加上可爱属性
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

int make_block(int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFD)) < 0) return -1;
	flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}


