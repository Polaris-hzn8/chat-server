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
#include "include/socket.h"

// 将ifd文件描述符 加入到从反应堆中
int add_to_subreactor(int epollfd, int ifd) {
	struct epoll_event ev;
	ev.data.fd = ifd;
	ev.events = EPOLLIN | EPOLLET;//使用边缘模式
	make_nonblock(ifd);
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ifd, &ev) == -1) return -1;
	DBG(L_RED"<Sub Reactor>"NONE" : fd<%d> was added in sub reactor<%d>. (in add_to_subreactor)\n", ifd, epollfd);
	return 0;
}

void *sub_reactor_task(void *arg) {
	// 从反应堆逻辑处理
	int subepollfd = *(int *)arg;
	DBG(L_RED"<Sub Reactor>"NONE" : in sub reactor subepollfd = %d.\n", subepollfd);
	
	// 利用反应堆模式epoll进行事件分发
	struct epoll_event events[S_MAXEVENTS];
	for (;;) {
		sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);

		//DBG(YELLOW"<in sub reactor loop : start\n"NONE);
		// 1.epoll_wait开始监听
		int nfds = epoll_pwait(subepollfd, events, S_MAXEVENTS, -1, &sigset);//nfds epoll检测到事件发生的个数 -1永久阻塞
		if (nfds < 0) {
			DBG(L_RED"<Sub Reactor Err>"NONE" : a error acuurL_RED in epoll_wati of sub reactor %d.\n", subepollfd);
			continue;
		}

		// 2.对epoll_wait监测到发生的所有事件进行遍历
		for (int i = 0; i < nfds; ++i) {
			// DBG(YELLOW"<in sub reactor loop : for each event\n"NONE);
			int ifd = events[i].data.fd;
			// 与主反应堆不同 从反应堆不需要对server_listen文件描述符 进行判定 只需要执行自己的工作即可
			// 接收用户发来的数据 进行处理操作

			// 3.对监测到事件event的文件描述符ifd的读写事件，从反应堆对客户的具体业务逻辑进行处理
			struct wechat_msg msg;
			bzero(&msg, sizeof(msg));

			// 4.将用户发送的数据进行接收
			int ret = recv(ifd, (void *)&msg, sizeof(msg), 0);
			
			// 5.如果客户端关闭 或文件传输出现问题 就关闭对应的文件描述符
			if ( ret <= 0) {
				// 设置了非阻塞IO 故当没有数据时可能返回-1 需要进行特判 即errno != EAGAIN
				// 客户端关闭了或文件出问题了 就关闭对应的文件描述符（普通 ifd）
				// 用户登出 并关闭为其服务的文件描述符 ifd
				epoll_ctl(subepollfd, EPOLL_CTL_DEL, ifd, NULL);
				users[ifd].isOnline = 0;
				close(ifd);
				DBG(L_RED"<Sub Reactor>"NONE" : client connection of %d on %d is closed.\n", ifd, subepollfd);
				continue;
			}

			// 6.文件接收到的大小有问题
			if (ret != sizeof(msg)) {
				DBG(L_RED"<Sub Reactor>"NONE" : msg size err! ret: %d sizeof(msg): %ld\n", ret, sizeof(msg));
				continue;
			}

			users[ifd].isOnline = 5;

			// 7.根据用户发送的信息进行对应的逻辑处理
			if (msg.cid & WECHAT_ACK && msg.cid & WECHAT_HEART) {
				// 7.1 客户端发送的心跳包确认消息
				DBG(RED"Ack for heart_beat\n"NONE);
			} else if (msg.cid & WECHAT_FIN) {
				// 7.2 用户选择断开连接 用户下线
				msg.cid = WECHAT_SYS;
				sprintf(msg.content, "你的好友 %s 下线了.", msg.from);
				broadcast(&msg);
				epoll_ctl(subepollfd, EPOLL_CTL_DEL, ifd, NULL);
				users[ifd].isOnline = 0;
				close(ifd);
				actUser--;
				DBG(L_RED"<Sub Reactor>"NONE" : current active user number: %d.\n", actUser);
			} else if (msg.cid & WECHAT_WALL) {
				// 7.3 用户选择广播消息
				DBG(L_RED"<Sub Reactor>"NONE" : a user choose to broadcast message.\n");
				DBG("%s : %s\n", msg.from, msg.content);
				msg.cid = WECHAT_ACK;
				broadcast(&msg);
			} else if (msg.cid & WECHAT_MSG) {
				// 7.4 用户选择私发消息
				DBG(L_RED"<Sub Reactor>"NONE" : a user choose to srcret message.\n");
				msg.cid = WECHAT_ACK;
				int ret = secret(&msg);
				if (!ret) {
					/* 用户不在线 或 用户不存在 */
					msg.cid = WECHAT_SYS;
					sprintf(msg.content, "用户 %s 已下线, 暂时无法发送私聊消息. ", msg.to);
					send(ifd, (void *)&msg, sizeof(msg), 0);
				}
			} else if (msg.cid & WECHAT_ACT) {
				// 7.5 用户选择查询当前在线人数
				DBG(L_RED"<Sub Reactor>"NONE" : a user choose to query active user.\n");
				msg.cid = WECHAT_ACT;
				msg.actUser = actUser;
				send(ifd, (void *)&msg, sizeof(msg), 0);
			} else {
				// 7.6 报文数据有误

			}

			// 8.在从反应堆中引入任务队列 优化服务器处理性能
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

	}

}

