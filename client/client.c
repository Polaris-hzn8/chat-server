/*************************************************************************
	> File Name: client.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Mon 17 Apr 2023 09:52:32 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/socket.h"
#include "include/info.h"
#include "include/read.h"
#include "include/imfunc.h"

const char *conf_file = "./config.conf";

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char **argv) {
	// 1.解析命令行参数
	int opt;
	int mode = 0; // 0 表示注册 1 表示登录
	int sex = -1;
	int server_port = 0;
	char server_ip[20] = {0};
	char name[50] = {0};
	while ((opt = getopt(argc, argv, "p:h:m:n:s:")) != -1) {
		switch (opt) {
		case 'p':
			server_port = atoi(optarg);
			break;
		case 'h':
			strcpy(server_ip, optarg);
			break;
		case 'm':
			mode = atoi(optarg);
			break;
		case 'n':
			strcpy(name, optarg);
			break;
		case 's':
			sex = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage : %s -p port -h server_port -m mode -n name -s sex .\n", argv[0]);
			exit(1);
		}
	}

    // 2.判断config文件是否存在
    if (access(conf_file, R_OK)) {
        fprintf(stderr, RED"<Error>"NONE" : config file don't exist!\n");
        exit(1);
    }

    // 3.若在选项中没有指定参数 则读取配置文件获取配置参数
    if (!server_port) {
        char *info;
        if ((info = get_conf_value(conf_file, "SERVER_PORT")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        server_port = atoi(info);
    }
	if (!strlen(server_ip)) {
        char *info;
        if ((info = get_conf_value(conf_file, "SERVER_IP")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        strcpy(server_ip, info);
    }
	if (!strlen(name)) {
        char *info;
        if ((info = get_conf_value(conf_file, "NAME")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        strcpy(name, info);
    }
	if (sex == -1) {
        char *info;
        if ((info = get_conf_value(conf_file, "SEX")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        sex = atoi(info);
    }
    DBG(YELLOW"<D>"NONE" : config file read success.\n");
	DBG(YELLOW"server_ip=%s\n"NONE, server_ip);
	DBG(YELLOW"server_port=%d\n"NONE, server_port);
	DBG(YELLOW"name=%s\n"NONE, name);
	DBG(YELLOW"sex=%d\n"NONE, sex);

	// 4.连接服务端
	int con_sockfd;
	if ((con_sockfd = socket_connect(server_ip, server_port)) < 0) handle_error("socket_connect");
	DBG(YELLOW"<D>"NONE" : connect to server %s:%d <%d>success.\n", server_ip, server_port, consockfd);

	// 好友logout提示
	temp.fd = con_sockfd;
	strcpy(temp.name, name);
	signal(SIGINT, logout);

	// 5.将注册与登录信息发送给客户端 0 表示注册 1 表示登录
	struct wechat_msg msg;
	bzero(&msg, sizeof(msg));
	strcpy(msg.from, name);
	msg.sex = sex;
	if (mode == 0) {
		msg.cid = WECHAT_SIGNUP;	// user signup
	} else {
		msg.cid = WECHAT_SIGNIN;	// user signin
	}
	send(con_sockfd, (void *)&msg, sizeof(msg), 0);

	// 6.利用select函数处理客户端与服务端之间的通信 发送登录or注册请求 等待2秒看服务端是否有回复
	fd_set rfds;
	FD_ZERO(&rfds);					// 初始化文件描述符集合
	FD_SET(con_sockfd, &rfds);		// 使用FD_SET函数将conn_socket添加到rfds集合中
	struct timeval tv;				// 使用 struct timeval 结构体 tv 来设置超时时间，2秒后 select 将返回
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	int ret;
	// 6.1 调用select函数监视con_sockfd是否有可读事件
	ret = select(con_sockfd + 1, &rfds, NULL, NULL, &tv);
	// 6.2 返回值小于等于0 发送的登录或注册消息在2秒内没有得到服务器响应
	if (ret <= 0) {
		fprintf(stderr, RED"<Err>"NONE" : Server no response.\n");
		exit(1);
	}

	// 6.3 发送的登录或注册消息在2秒内得到了服务器响应 select返回值大于0得到了响应 开始接受服务端发送的消息
	bzero(&msg, sizeof(msg));
	ret = recv(con_sockfd, (void *)&msg, sizeof(msg), 0);
	if (ret <= 0) {
		// 服务端关闭 ret == 0 ; recv出错 ret < 0
		fprintf(stderr, RED"<Err>"NONE" : server loss connection.\n");
		exit(1);
	}

	// 6.4 服务器返回请求成功响应
	if (msg.cid & WECHAT_ACK) {
		// msg.type == WECHAT_ACK
		DBG(GREEN"<Success>"NONE" : server return a success.\n");
		if (!mode) {
			// 进入注册逻辑
			printf(GREEN"user choose to register for a account.\n"NONE);
			exit(0);
		} else {
			// 进入登录逻辑
			printf(GREEN"user choose to login.\n"NONE);
		}
	} else {
		// msg.type == WECHAT_FIN
		DBG(RED"<Failure>"NONE" : server return a failure.\n");
		close(con_sockfd);
		exit(1);
	}

	// 7.用户成功登录后 进入聊天室的主逻辑：接收消息 与 发送消息
	// 有必要将接受消息 与 发送消息的逻辑分离 否则在输入消息的过程中就会被接受到的消息打断
	// 创建另外一个线程用于循环接收消息 一旦接受到消息就打印输出在屏幕上 而主级进程用于发送消息
	pthread_t tid;
	pthread_create(&tid, NULL, client_recv, (void *)&con_sockfd);

	printf("Start messaging your idea: \n");
	while (1) {
		char buff[1024] = {0};
		scanf("%[^\n]s", buff); getchar();
		if (strlen(buff)) {
			bzero(&msg, sizeof(msg));
			if (strlen(buff) == 1 && buff[0] == '#') {
				// 客户端选择查看当前系统在线人数
				msg.cid = WECHAT_ACT;
				send(con_sockfd, (void *)&msg, sizeof(msg), 0);
			} else if (buff[0] == '@') {
				// 客户端选择私聊发送消息
				if (!strstr(buff, " ")) {
					// 简单的私聊消息命令格式检查
					fprintf(stderr, "Usage : @[username] [message]\n");
					continue;
				}
				msg.cid = WECHAT_MSG;
				//将命令中消息发送目标的名字 拷贝到msg.to中
				strncpy(msg.to, buff + 1, strchr(buff, ' ') - buff - 1);
				//DBG(RED"<target>"NONE" : target name: %s\n", msg.to);
				strcpy(msg.from, name);
				strncpy(msg.content, buff + strlen(msg.to) + 2, strlen(buff) - strlen(msg.to) - 2);
				//DBG(RED"<target>"NONE" : message content: %s\n", msg.content);
				send(con_sockfd, (void *)&msg, sizeof(msg), 0);
			} else {
				//客户端选择公聊发送消息
				msg.cid = WECHAT_WALL;
				strcpy(msg.from, name);
				strcpy(msg.content, buff);
				send(con_sockfd, (void *)&msg, sizeof(msg), 0);
			}
		}
	}
	return 0;
}

