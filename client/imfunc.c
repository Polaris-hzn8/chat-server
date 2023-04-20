/*************************************************************************
	> File Name: imfunc.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 04:48:31 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/imfunc.h"
#include "include/color.h"
#include "include/info.h"

extern int consockfd;

void logout(int signum) {
	/* 客户端主动选择logout 关闭连接 */
	struct wechat_msg msg;
	msg.type = WECHAT_FIN;
	strcpy(msg.from, temp.name);
	int consockfd = temp.fd;
	send(temp.fd, (void *)&msg, sizeof(msg), 0);
	close(consockfd);
	printf(YELLOW"Bye!~"NONE);
	exit(0);
}

void *client_recv(void *arg) {
	/* 新建1个线程用于处理客户端的信息读取 一旦有消息recv就立即将消息打印在屏幕上 */
	int sockfd = *(int *)arg;
	struct wechat_msg msg;
	while (1) {
		bzero(&msg, sizeof(msg));
		int ret = recv(sockfd, &msg, sizeof(msg), 0);
		if (ret <= 0) {
			DBG(L_CYAN"<Client>"NONE" : server closed the connection.\n");
			exit(1);
		}
		if (msg.type & WECHAT_HEART) {
			//printf(BLINK"receive a heart package from server.\n"NONE);
			/* 收到心跳包对服务端进行响应 */
			struct wechat_msg response;
			response.type = WECHAT_ACK | WECHAT_HEART;
			send(sockfd, &response, sizeof(response), 0);
		} else if (msg.type & WECHAT_SYS) {
			/* 系统消息 */
			printf(RED"SysInfo : %s\n"NONE, msg.content);
		} else if (msg.type & WECHAT_ACT) {
			/* 系统在线人数查询 */
			printf(RED"SysInfo : 当前在线活跃人数为: %d 人.\n"NONE, msg.actUser);
		} else {
			printf("%s : %s\n", msg.from, msg.content);
		}
	}
}


