/*************************************************************************
	> File Name: wechat.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Sat 08 Apr 2023 11:32:52 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/sub_reactor.h"
#include "include/info.h"
#include "include/imfunc.h"

int add_to_subreactor(int epollfd, int fd) {
	/* 将fd文件描述符 加入到从反应堆中 */
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN | EPOLLET;//使用边缘模式
	make_nonblock(fd);
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) return -1;
	DBG(L_RED"<Sub Reactor>"NONE" : fd<%d> was added in sub reactor<%d>.\n", fd, epollfd);
	return 0;
}

void *sub_reactor(void *arg) {
	/* 新建1个线程用于处理从反应堆逻辑 */
	int subepollfd = *(int *)arg;
	DBG(L_RED"<Sub Reactor>"NONE" : in sub reactor %d.\n", subepollfd);
	
	//从反应堆主逻辑处理开始
	//1.利用反应堆模式epoll进行事件分发
	int sockfd;
	// 1-1 epoll_create
	//if ((epollfd = epoll_create(1)) < 0) handle_error("epoll_create");//文件描述符被占用完就可能会出错

	struct epoll_event events[S_MAXEVENTS], ev;
	for (;;) {
		sigset_t st;
        sigemptyset(&st);
        sigaddset(&st, SIGALRM);

		//DBG(YELLOW"<in sub reactor loop : start\n"NONE);
		// 1-2 epoll_wait开始监听
		int nfds = epoll_pwait(subepollfd, events, S_MAXEVENTS, -1, &st);//nfds epoll检测到事件发生的个数 -1永久阻塞
		if (nfds < 0) {
			DBG(L_RED"<Sub Reactor Err>"NONE" : a error acuurL_RED in epoll_wati of sub reactor %d.\n", subepollfd);
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			//DBG(YELLOW"<in sub reactor loop : for each event\n"NONE);
			// 1-2-1 对epoll_wait监测到发生的所有事件进行遍历，进行事件分发
			int fd = events[i].data.fd;
			/* 与主反应堆不同 从反应堆不需要对server_listen文件描述符 进行判定 只需要执行自己的工作即可 */

			/* 接收用户发来的数据 进行处理操作 */
			// 1-2-2 对监测到事件event的文件描述符fd进行操作，从反应堆对客户的具体业务逻辑进行处理
			/* 聊天系统从反应堆逻辑： */
			//（1）将用户发送的数据进行接收
			struct wechat_msg msg;
			bzero(&msg, sizeof(msg));
			int ret = recv(fd, (void *)&msg, sizeof(msg), 0);
			
			//（2）如果客户端关闭 或文件传输出现问题 就关闭对应的文件描述符
			if ( ret <= 0) {
				/* 设置了非阻塞IO 故当没有数据时可能返回-1 需要进行特判 即errno != EAGAIN*/
				/* 客户端关闭了或文件出问题了 就关闭对应的文件描述符（普通 fd） */
				/* 用户登出 并关闭为其服务的文件描述符fd */
				epoll_ctl(subepollfd, EPOLL_CTL_DEL, fd, NULL);
				users[fd].isOnline = 0;
				close(fd);
				DBG(L_RED"<Sub Reactor>"NONE" : connection of %d on %d is closed.\n", fd, subepollfd);
				continue;
			}
			if (ret != sizeof(msg)) {
				/* 文件接收到的大小有问题 */
				DBG(L_RED"<Sub Reactor>"NONE" : msg size err! ret:%d sizeof(msg):%d\n", ret, sizeof(msg));
				continue;
			}

			users[fd].isOnline = 5;
			//（3）根据用户发送的信息进行对应的逻辑处理
			if (msg.type & WECHAT_ACK && msg.type & WECHAT_HEART) {
				DBG(RED"Ack for heart_beat"NONE);



			} else if (msg.type & WECHAT_WALL) {
				/* （3-1）用户选择广播消息 */
				DBG(L_RED"<Sub Reactor>"NONE" : a user choose to broadcast message.\n");
				DBG("%s : %s\n", msg.from, msg.content);
				msg.type = WECHAT_ACK;
				broadcast(msg);
			} else if (msg.type & WECHAT_MSG) {
				/* （3-2）用户选择私发消息 */
				/* 将该成功登录的用户fd文件描述符 加入到从反应堆中 登录后的逻辑交给从反应堆处理 */
				DBG(L_RED"<Server>"NONE" : a user choose to sign in\n");
				msg.type = WECHAT_ACK;
				send(fd, (void *)&msg, sizeof(msg), 0);
				strcpy(users[fd].name, msg.from);
				users[fd].isOnline = 1;
				users[fd].sex = msg.sex;
				users[fd].fd = fd;
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


		}//for
	}//for

}

