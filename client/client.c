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

const char *config = "./wechat.conf";

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char **argv) {
	int opt;
	int mode = 0;//客户端启动模式 0 表示注册 1 表示登录
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

    //1.判断config文件是否存在
    if (access(config, R_OK)) {
        fprintf(stderr, RED"<Error>"NONE" : config file don't exist!\n");
        exit(1);
    }

    //2.若在选项中没有指定参数 则读取配置文件获取配置参数
    if (!server_port) {
        char *info;
        if ((info = get_conf_value(config, "SERVER_PORT")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        server_port = atoi(info);
    }
	if (!strlen(server_ip)) {
        char *info;
        if ((info = get_conf_value(config, "SERVER_IP")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        strcpy(server_ip, info);
    }
	if (!strlen(name)) {
        char *info;
        if ((info = get_conf_value(config, "NAME")) == NULL) {
            fprintf(stderr, RED"<Error>"NONE" this setting info is not set in the config file!\n");
            exit(1);
        }
        strcpy(name, info);
    }
	if (sex == -1) {
        char *info;
        if ((info = get_conf_value(config, "SEX")) == NULL) {
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

	//3.连接服务端
	int consockfd;
	if ((consockfd = socket_connect(server_ip, server_port)) < 0) handle_error("socket_connect");
	DBG(YELLOW"<D>"NONE" : connect to server %s:%d <%d>success.\n", server_ip, server_port, consockfd);


	// 好友logout提示
	temp.fd = consockfd;
	strcpy(temp.name, name);
	signal(SIGINT, logout);


	struct wechat_msg msg;
	//4.将注册与登录信息发送给客户端
	bzero(&msg, sizeof(msg));
	strcpy(msg.from, name);
	msg.sex = sex;
	if (mode == 0) {
		/* user signup */
		msg.type = WECHAT_SIGNUP;
	} else {
		/* user signin */
		msg.type = WECHAT_SIGNIN;
	}
	send(consockfd, (void *)&msg, sizeof(msg), 0);

	//5.利用select处理客户端发送登录or注册请求 等待2秒看服务端是否有回复
	//5-1 发送的登录或注册消息在2秒内没有得到服务器响应
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(consockfd, &rfds);
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (select(consockfd + 1, &rfds, NULL, NULL, &tv) <= 0) {
		fprintf(stderr, RED"<Err>"NONE" : Server no response.\n");
		exit(1);
	}

	//5-2 发送的登录或注册消息在2秒内得到了服务器响应 select返回值大于0得到了响应 开始接受服务端发送的消息
	bzero(&msg, sizeof(msg));
	int ret = recv(consockfd, (void *)&msg, sizeof(msg), 0);
	if (ret <= 0) {
		/* 服务端关闭 ret == 0 ; recv出错 ret < 0 */
		fprintf(stderr, RED"<Err>"NONE" : server loss connection.\n");
		exit(1);
	}
	if (msg.type & WECHAT_ACK) {
		/* msg.type == WECHAT_ACK */
		DBG(GREEN"<Success>"NONE" : server return a success.\n");
		if (!mode) {
			/* 进入注册逻辑 */
			printf(GREEN"please login after this.\n"NONE);
			exit(0);
		}
	} else {
		/* msg.type == WECHAT_FIN */
		DBG(RED"<Failure>"NONE" : server return a failure.\n");
		close(consockfd);
		exit(1);
	}

	//6.进入聊天室的主逻辑：接收消息 与 发送消息
	/* 有必要将接受消息 与 发送消息的逻辑分离 否则在输入消息的过程中就会被接受到的消息打断 */
	/* 创建另外一个线程用于循环接收消息 一旦接受到消息就打印输出在屏幕上 */
	/* 主级进程用于发送消息 */
	pthread_t tid;
	pthread_create(&tid, NULL, client_recv, (void *)&consockfd);

	printf("Start messaging your idea: \n");
	while (1) {
		char buff[1024] = {0};
		scanf("%[^\n]s", buff); getchar();
		if (strlen(buff)) {
			bzero(&msg, sizeof(msg));
			if (strlen(buff) == 1 && buff[0] == '#') {
				//客户端选择查看当前系统在线人数
				msg.type = WECHAT_ACT;
				send(consockfd, (void *)&msg, sizeof(msg), 0);
			} else if (buff[0] == '@') {
				//客户端选择私聊发送消息
				if (!strstr(buff, " ")) {
					/* 简单的私聊消息命令格式检查 */
					fprintf(stderr, "Usage : @[username] [message]\n");
					continue;
				}
				msg.type = WECHAT_MSG;
				//将命令中消息发送目标的名字 拷贝到msg.to中
				strncpy(msg.to, buff + 1, strchr(buff, ' ') - buff - 1);
				//DBG(RED"<target>"NONE" : target name: %s\n", msg.to);
				strcpy(msg.from, name);
				strncpy(msg.content, buff + strlen(msg.to) + 2, strlen(buff) - strlen(msg.to) - 2);
				//DBG(RED"<target>"NONE" : message content: %s\n", msg.content);
				send(consockfd, (void *)&msg, sizeof(msg), 0);
			} else {
				//客户端选择公聊发送消息
				msg.type = WECHAT_WALL;
				strcpy(msg.from, name);
				strcpy(msg.content, buff);
				send(consockfd, (void *)&msg, sizeof(msg), 0);
			}
		}
	}
	return 0;
}

