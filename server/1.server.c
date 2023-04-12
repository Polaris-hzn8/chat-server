/*************************************************************************
	> File Name: 1.server.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Mon 03 Apr 2023 04:40:15 PM CST
 ************************************************************************/

#include "head.h"
#include "common.h"
#include "wechat.h"

const char *config = "./wechatd.conf";

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[]) {
    int opt;
    int port = 0;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage : %s -p port.\n", argv[0]);
                exit(1);
        }
    }
    //1.判断config文件是否存在
    if (access(config, R_OK)) {
        fprintf(stderr, RED"<Error>"NONE" config file don't exist!\n");
        exit(1);
    }

    //2.若在选项中没有指定port端口的值 则读取配置文件获取port配置值
    if (!port) {
        char *info;
        if ((info = get_conf_value(config, "PORT")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        port = atoi(info);
    }
    DBG(YELLOW"port = %d\n"NONE, port);
    DBG(YELLOW"<D>"NONE" : config file read success.\n");

    //3.创建socket连接
    int server_listen, sockfd;
    if ((server_listen = socket_create(port)) < 0)handle_error("socket_create");
    DBG(YELLOW"<D>"NONE" : server_listen is listening on port %d.\n", port);

    //4.创建主反应堆 从反应堆
    int epollfd, subefd1, subefd2;
    if (epollfd = epoll_create(1) < 0) handle_error("epollfd_create");
    if (subefd1 = epoll_create(1) < 0) handle_error("subefd1_create");
    if (subefd2 = epoll_create(1) < 0) handle_error("subefd2_create");
    DBG(YELLOW"<D>"NONE" : Main reactor and two sub reactors created.\n");

    //5.为从反应堆创建线程
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, sub_reactor, (void *)&subefd1);
    pthread_create(&tid2, NULL, sub_reactor, (void *)&subefd2);
    DBG(YELLOW"<D>"NONE" : Two sub reactor threads created.\n");

    usleep(10);
    return 0;
}



