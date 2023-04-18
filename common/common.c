/*************************************************************************
	> File Name: common.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Thu 30 Mar 2023 04:23:36 PM CST
 ************************************************************************/

#include "head.h"
#include "common.h"

int socket_create(int port) {
	int sockfd;
	struct sockaddr_in server;
	//1.创建套截字
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	//2.绑定套接字与结构体信息
	server.sin_family = AF_INET;//ipv4
	server.sin_port = htons(port);//host to net short 本地字节序的短整型转换为网络字节序
	server.sin_addr.s_addr = htonl(INADDR_ANY);//host to net short 本地字节序的长整型转换为网络字节序 INADDR_ANY监听任何一个地址 0.0.0.0
	int reuse = 1; setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int));//服务端地址重用
	if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) return -1;

	//3.将主动套接字转为被动
	if (listen(sockfd, 20) < 0) return -1;// 20: backlog正在连接的连接序列有多少个
	return sockfd;
}

int socket_connect(const char *ip, int port) {
	int sockfd;
	//1.客户端打开一个socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	//2.定义结构体用于绑定端口号、ip地址（存放服务端的具体信息）
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);//端口号
	server.sin_addr.s_addr = inet_addr(ip);//ip地址

	//3.建立连接connection
	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) return -1;
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

//读取并获得conf文件中的配置信息
char *get_conf_value(const char *filename, const char *key) {
    FILE *fp;
    char *line = NULL;//获取的行
    char *sub = NULL;//找到的目标下标指针
    ssize_t len = 0, nread = 0;
    bzero(ans, sizeof(ans));
    //1.打开配置文件
    if ((fp = fopen(filename, "r")) == NULL) return NULL;
    
	//2.逐行读取文件中的信息 进行目标key的查找
    while ((nread = getline(&line, &len, fp)) != -1) {
        if ((sub = strstr(line, key)) == NULL) continue;
        if (sub == line && line[strlen(key)] == '=') {
            /* sub == line && line[strlen(key)]为等于号 */
            strcpy(ans, line + strlen(key) + 1);
            if (ans[strlen(ans) - 1] == '\n') {
                ans[strlen(ans) - 1] = '\0';//行尾的换行符处理
            }
        }
    }
    fclose(fp);
    free(line);
    if (!strlen(ans)) return NULL;
    return ans;
}


