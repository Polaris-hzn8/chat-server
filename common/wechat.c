/*************************************************************************
	> File Name: wechat.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Sat 08 Apr 2023 11:32:52 PM CST
 ************************************************************************/

#include "head.h"
#include "common.h"
#include "wechat.h"

int add_to_subreactor(int epollfd, int fd) {
	/* 将fd文件描述符 加入到从反应堆中 */
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET;//使用边缘模式
	make_nonblock(fd);
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) return -1;
	DBG(L_RED"<Sub Reactor>"NONE" : fd<%d> was added in sub reactor<> %d.\n", fd, epollfd);
	return 0;
}

void *sub_reactor(void *arg) {
	/* 新建1个线程用于创建从反应堆 并处理从反应堆逻辑 */
	int subfd = *(int *)arg;
	while (1) {
		DBG(L_RED"<Sub Reactor>"NONE" : in sub reactor %d.\n", subfd);
		sleep(3000);
	}
}

void *client_recv(void *arg) {
	/* 新建1个线程用于处理客户端的信息读取 一旦有消息recv就立即将消息打印在屏幕上 */
	int sockfd = *(int *)arg;
	struct wechat_msg msg;
	while (1) {
		bzero(&msg, sizeof(msg));
		int ret = recv(sockfd, &msg, sizeof(msg), 0);
		printf("%s : %s\n", msg.from, msg.msg);
	}
}

