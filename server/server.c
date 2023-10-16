/*************************************************************************
	> File Name: 1.server.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Mon 03 Apr 2023 04:40:15 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/socket.h"
#include "include/read.h"
#include "include/info.h"
#include "include/sub_reactor.h"
#include "include/thread_pool.h"
#include "include/imfunc.h"

const char *conf_file = "./config.conf";

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[]) {
    // 1.解析命令行参数
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

    // 2.判断conf文件是否存在
    if (access(conf_file, R_OK)) {
        fprintf(stderr, RED"<Config file Error>"NONE" config file don't exist!\n");
        exit(1);
    }

    // 3.若在命令行选项中没有指定port端口的值 则读取配置文件获取port配置值
    if (!port) {
        char *info;
        if ((info = get_conf_value(conf_file, "PORT")) == NULL) {
            fprintf(stderr, RED"<Config file Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        port = atoi(info);
    }
    DBG(YELLOW"port = %d\n"NONE, port);
    DBG(YELLOW"<D>"NONE" : config file read success.\n");

    // 4.通过 socket_create 函数创建用于用于监听连接的套接字 socket_listen 
    // 该连接字通常用于等待客户端的连接请求 并在指定端口上进行监听
    int server_listen;
    if ((server_listen = socket_create(port)) < 0) handle_error("socket_create");
    DBG(YELLOW"<D>"NONE" : server_listen is listening on port %d.\n", port);

    // 初始化服务端用于存储用户的数组
    users = (struct wechat_user*)calloc(MAXUSERS, sizeof(struct wechat_user));

    // 心跳机制
    // 使用间隔计时器setitimer 来触发定时器信号SIGALRM 每隔10s触发一次
    // 该机制用于检测客户端或其他网络设备是否仍然在线 当信号触发时会执行heart_beat函数
    struct itimerval itv;
    itv.it_interval.tv_sec = 10;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 10;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);//每隔10s释放一个信号
    signal(SIGALRM, heart_beat);

    // 5.创建主反应堆与从反应堆
    // 使用epoll_create函数创建三个epoll实例
    // 主反应堆通常用于处理新的客户端连接请求 从反应堆用于处理已建立连接的数据传输 这种分离可以提高网络服务的并发性能
    if ((epollfd1 = epoll_create(1)) < 0) handle_error("epollfd_create");//反应堆1
    if ((epollfd2 = epoll_create(1)) < 0) handle_error("subefd1_create");//反应堆2
    if ((epollfd3 = epoll_create(1)) < 0) handle_error("subefd2_create");//反应堆3
    DBG(YELLOW"<D>"NONE" : main reactor and two sub reactors created.\n");

    // 6.创建两个工作线程 每个线程用于处理一个从反应堆（epoll实例）这些反应堆通常用于处理已经建立连接的数据传输
    // 通过 pthread_create 函数创建了两个线程 分别存储在tid2和tid3变量中
    // 这两个线程将用于执行从反应堆的任务 sub_reactor_task 并且传入的参数为 epollfd2 和 epollfd3
    pthread_t tid2, tid3;
    pthread_create(&tid2, NULL, sub_reactor_task, (void *)&epollfd2);    //反应堆2的线程
    pthread_create(&tid3, NULL, sub_reactor_task, (void *)&epollfd3);    //反应堆3的线程
    DBG(YELLOW"<D>"NONE" : two sub reactor threads has been created to handle task:sub_reactor.\n");
    
    // 7.反应堆模式epoll进行事件分发
	int con_sockfd;
    // 7-1 epoll_ctl将server_listen文件描述符注册到epoll实例中
	struct epoll_event events[M_MAXEVENTS], ev;
	ev.data.fd = server_listen;                              //关注的文件描述符
	ev.events = EPOLLIN;                                     //关注的事件、需要注册的事件
	epoll_ctl(epollfd1, EPOLL_CTL_ADD, server_listen, &ev);  //注册操作
	DBG(YELLOW"<D> : server_listen is added to epollfd successfully.\n"NONE);
	
    // 循环的判断有个文件描述符变得就绪
	for (;;) {
        // 心跳包sigset
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);

		// 7-2 epoll_wait开始监听
		int nfds = epoll_pwait(epollfd1, events, M_MAXEVENTS, -1, &sigset);     //nfds epoll检测到事件发生的个数
		if (nfds == -1) handle_error("epoll_wait");                             //可能被时钟中断 or 其他问题

        // 7-3 对epoll_wait监测到发生的所有事件进行遍历，进行事件分发
		for (int i = 0; i < nfds; ++i) {
			int ifd = events[i].data.fd;
			if (ifd == server_listen && (events[i].events & EPOLLIN)) {
				// 7-3-1 返回的可读文件描述符fd为server_listen 表示已经有新的客户端尝试与服务端进行3次握手
				// events[i].events & EPOLLIN 表示至少有一个可读

				// 将accept到的文件描述符注册到epoll实例中，实现文件描述符监听
				if ((con_sockfd = accept(server_listen, NULL, NULL)) < 0) handle_error("accept");
				// 如果是实际应用情况，如果出现错误应该想办法处理错误，并恢复实际业务
				// clients[sockfd] = sockfd;//文件描述符作为数组下标 存储新出现的conn_socket文件描述符
				ev.data.fd = con_sockfd;
				ev.events = EPOLLIN;                                    // 设置为边缘触发模式
				make_nonblock(con_sockfd);                              // 设置为非阻塞
				epoll_ctl(epollfd1, EPOLL_CTL_ADD, con_sockfd, &ev);    // 将连接的文件描述符注册到反应堆中
			} else {
                // 7-3-2 返回的可读文件描述符fd不是server_listen 表示接收到用户发来的数据、验证数据并完成具体的操作

				// 聊天系统主反应堆逻辑
                // 对监测到事件event的文件描述符fd进行操作，具体业务逻辑交给从反应堆处理，主反应堆只负责登录注册与登出业务
                // step1：将用户发送的数据进行接收
                struct wechat_msg msg;
                bzero(&msg, sizeof(msg));
                int ret = recv(ifd, (void *)&msg, sizeof(msg), 0);//从ifd中接受数据到msg中
                
                // step2：如果客户端关闭 就关闭对应的文件描述符
                if ( ret <= 0) {
                    // 客户端关闭了或文件出问题了 关闭对应的文件描述符（包括建立连接用的server_listen 与 普通ifd）
                    // 用户登出 并关闭为其服务的文件描述符fd
                    epoll_ctl(epollfd1, EPOLL_CTL_DEL, ifd, NULL);
                    close(ifd);
                    DBG(RED"<Master Reactor>"NONE" : connection of %d on %d is closed.\n", ifd, epollfd1);
                    continue;
                }

                // step3：如果文件传输大小出现问题 关闭对应的文件描述符
                if (ret != sizeof(msg)) {
                    // 文件接收到的大小有问题
                    DBG(RED"<Master Reactor>"NONE" : msg size err! ret: %d sizeof(msg): %ld\n", ret, sizeof(msg));
                    continue;
                }

                // step4：根据用户发送的信令 进行相应的逻辑处理
                if (msg.cid & WECHAT_SIGNUP) {
                    DBG(RED"<Master Reactor>"NONE" : a user choose to sign up\n");
                    // 0x10 用户选择注册 更新用户信息到数据库中 判断是否可以注册
                    msg.cid = WECHAT_ACK;
                    send(ifd, (void *)&msg, sizeof(msg), 0);
                } else if (msg.cid & WECHAT_SIGNIN) {
                    DBG(RED"<Master Reactor>"NONE" : a user choose to login in\n");
                    // 0x20 用户选择登录 用户上线 判断密码是否正确 验证用户是否重复登录
                    // 将该成功登录的用户fd文件描述符 加入到从反应堆中 登录后的逻辑交给从反应堆处理
                    actUser++;
                    DBG(L_RED"<Sub Reactor>"NONE" : current active user number: %d.\n", actUser);

                    msg.cid = WECHAT_ACK;
                    send(ifd, (void *)&msg, sizeof(msg), 0);
                    strcpy(users[ifd].name, msg.from);
                    users[ifd].fd = ifd;
                    users[ifd].isOnline = 5;
                    users[ifd].sex = msg.sex;

                    // 利用负载均衡算法 决定将任务交给哪个从反应堆处理 事务转手（注册到哪个反应堆的epoll实例中）
                    int whichsub = msg.sex ? epollfd2 : epollfd3;
                    epoll_ctl(epollfd1, EPOLL_CTL_DEL, ifd, NULL);  //将具体的事务ifd文件描述符从主反应堆中删除
                    add_to_subreactor(whichsub, ifd);               //将具体的事务ifd文件描述符加入到从反应堆中

                    // 新用户上线向其他用户广播消息
                    msg.cid = WECHAT_SYS;
                    sprintf(msg.content, "您的好友 %s 进入了聊天室，快和他打个招呼吧!\n", msg.from);
                    broadcast(&msg);
                } else {
                    // 报文数据有误

                }

			}

		}

	}
    return 0;
}



