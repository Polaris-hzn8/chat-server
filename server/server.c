/*************************************************************************
	> File Name: 1.server.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Mon 03 Apr 2023 04:40:15 PM CST
 ************************************************************************/

#include "../common/head.h"
#include "../common/common.h"
#include "../common/thread_pool.h"
#include "../common/wechat.h"

#define MAXEVENTS 5
#define MAXUSERS 1024

const char *config = "./wechatd.conf";
struct wechat_user *users;

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
    int server_listen;
    if ((server_listen = socket_create(port)) < 0)handle_error("socket_create");
    DBG(YELLOW"<D>"NONE" : server_listen is listening on port %d.\n", port);

    users = (struct wechat_user*)calloc(MAXUSERS, sizeof(struct wechat_user));

    //4.创建主反应堆 与 从反应堆
    int epollfd1, epollfd2, epollfd3;
    if ((epollfd1 = epoll_create(1)) < 0) handle_error("epollfd_create");//主反应堆
    if ((epollfd2 = epoll_create(1)) < 0) handle_error("subefd1_create");//从反应堆1
    if ((epollfd3 = epoll_create(1)) < 0) handle_error("subefd2_create");//从反应堆2
    DBG(YELLOW"<D>"NONE" : Main reactor and two sub reactors created.\n");

    //5.为从反应堆创建线程
    pthread_t tid2, tid3;
    pthread_create(&tid2, NULL, sub_reactor, (void *)&epollfd2);//从反应堆1的线程
    pthread_create(&tid3, NULL, sub_reactor, (void *)&epollfd3);//从反应堆2的线程
    DBG(YELLOW"<D>"NONE" : Two sub reactor threads created.\n");
    
    
    //6.利用反应堆模式epoll进行事件分发
	int sockfd;
	// 6-1 epoll_create
	//if ((epollfd = epoll_create(1)) < 0) handle_error("epoll_create");//文件描述符被占用完就可能会出错

    // 6-2 epoll_ctl将server_listen文件描述符注册到epoll实例中
	struct epoll_event events[MAXEVENTS], ev;
	ev.data.fd = server_listen;//关注的文件描述符
	ev.events = EPOLLIN;//关注的事件、需要注册的事件
	if (epoll_ctl(epollfd1, EPOLL_CTL_ADD, server_listen, &ev) < 0) handle_error("epoll_ctl");//注册操作
	DBG(YELLOW"<D> : server_listen is added to epollfd successfully.\n"NONE);
	
	for (;;) {
		// 6-3 epoll_wait开始监听
		int nfds = epoll_wait(epollfd1, events, MAXEVENTS, -1);//nfds epoll检测到事件发生的个数
		if (nfds == -1) handle_error("epoll_wait");//可能被时钟中断 or 其他问题

		for (int i = 0; i < nfds; ++i) {
			// 6-3-1 对epoll_wait监测到发生的所有事件进行遍历，进行事件分发
			int fd = events[i].data.fd;
			if (fd == server_listen && (events[i].events & EPOLLIN)) {
				/* 返回的fd为server_listen可读 表示已经有客户端进行3次握手了 */
				/* events[i].events & EPOLLIN 表示至少有一个可读 */
				// 6-3-2将accept到的文件描述符注册到epoll实例中，实现文件监听
				if ((sockfd = accept(server_listen, NULL, NULL)) < 0) handle_error("accept");
				//如果是实际应用情况，如果出现错误应该想办法处理错误，并恢复实际业务
				//clients[sockfd] = sockfd;//文件描述符作为数组下标 存储新出现的conn_socket文件描述符
				ev.data.fd = sockfd;
				ev.events = EPOLLIN;//设置为边缘触发模式
				make_nonblock(sockfd);
				if (epoll_ctl(epollfd1, EPOLL_CTL_ADD, sockfd, &ev) < 0) handle_error("epoll_ctl");
			} else {
				/* 返回的fd不是server_listen且可读 */
                /* 接收用户发来的数据 验证 以及操作 */
				// 6-3-3 对监测到事件event的文件描述符fd进行操作，具体业务逻辑交给从反应堆处理，主反应堆只负责登录注册与登出业务
                /* 聊天系统主反应堆逻辑： */
                //（1）将用户发送的数据进行接收
                struct wechat_msg msg;
                bzero(&msg, sizeof(msg));
                int ret = recv(fd, (void *)&msg, sizeof(msg), 0);
                
                //（2）如果客户端关闭 或文件传输出现问题 就关闭对应的文件描述符
                if ( ret <= 0) {
                    /* 客户端关闭了或文件出问题了 就关闭对应的文件描述符（包括建立连接用的 server_listen 与 普通 fd） */
                    /* 用户登出 并关闭为其服务的文件描述符fd */
                    epoll_ctl(epollfd1, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }
                if (ret != sizeof(msg)) {
                    /* 文件接收到的大小有问题 */
                    DBG(RED"<MsgErr>"NONE" : msg size err! ret:%d sizeof(msg):%d\n", ret, sizeof(msg));
                    continue;
                }

                //（3）根据用户发送的信息进行对应的逻辑处理
                if (msg.type & WECHAT_SIGNUP) {
                    /* （3-1）用户选择注册 更新用户信息到文件中 判断是否可以注册 */
                    DBG(RED"<Server>"NONE" : a user choose to sign up\n");
                    msg.type = WECHAT_ACK;
                    send(fd, (void *)&msg, sizeof(msg), 0);
                } else if (msg.type & WECHAT_SIGNIN) {
                    /* （3-2）用户选择登录 判断密码是否正确 验证用户是否重复登录 */
                    /* 将该成功登录的用户fd文件描述符 加入到从反应堆中 登录后的逻辑交给从反应堆处理 */
                    DBG(RED"<Server>"NONE" : a user choose to sign in\n");
                    msg.type = WECHAT_ACK;
                    send(fd, (void *)&msg, sizeof(msg), 0);
                    strcpy(users[fd].name, msg.from);
                    users[fd].isOnline = 1;
                    users[fd].sex = msg.sex;
                    users[fd].fd = fd;

                    /* 利用负载均衡算法 决定将任务交给哪个从反应堆处理（注册到哪个反应堆的epoll实例中） */
                    int whichsub = msg.sex ? epollfd2 : epollfd3;
                    add_to_subreactor(whichsub, fd);
                } else {
                    /* （3-3）报文数据有误 */
                }

				// if (events[i].events & EPOLLIN) {
				// 	/* 套接字属于就绪状态 有数据输入需要执行 */
				// 	task_queue_push(taskQueue, (void *)&clients[fd]);
				// 	//不可直接将fd传入 需要保证传入的fd值总是不同，创建clients[]数组保证每次传入的fd值不同
				// 	//当把地址作为参数传递给函数后（特别是在循环中），下一次fd的值会不断的被修改（传入的值是会变化的）
				// } else {
				// 	/* 套接字不属于就绪状态出错 将该事件的文件描述符从注册的epoll实例中删除 */
				// 	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
				// 	close(fd);
				// }
			}
		}//for
	}//for
    return 0;
}



