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
	return 0;
}

void *sub_reactor(void *arg) {
	/* 线程处理函数 */
	int subfd = *(int *)arg;
	while (1) {
		DBG(L_RED"<Sub Reactor>"NONE" : in sub reactor %d.\n", subfd);
		sleep(3000);
	}
}

