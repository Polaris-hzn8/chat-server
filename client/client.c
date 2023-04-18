/*************************************************************************
	> File Name: client.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Mon 17 Apr 2023 09:52:32 PM CST
 ************************************************************************/

#include "head.h"
#include "common.h"
#include "wechat.h"

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
	int sockfd;
	if ((sockfd = socket_connect(server_ip, server_port)) < 0) handle_error("socket_connect");
	DBG(YELLOW"<D>"NONE" : connect to server %s:%d <%d>success.\n", server_ip, server_port, sockfd);

	//4.进入登录或注册验证
	struct wechat_msg msg;
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
	send(sockfd, (void *)&msg, sizeof(msg), 0);

	return 0;
}

