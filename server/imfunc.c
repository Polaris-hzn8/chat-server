/*************************************************************************
	> File Name: imfunc.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 04:39:00 PM CST
 ************************************************************************/

#include "include/imfunc.h"

void heart_beat(int signum) {
	struct wechat_msg msg;
	msg.type = WECHAT_HEART;
	for (int i = 0; i < CUR_USER_NUMBER; ++i) {
		if (users[i].isOnline) {
			/* 为所有在线的用户发送心跳包 */
			send(users[i].fd, (void *)&msg, sizeof(msg), 0);
			users[i].isOnline--;
			if (users[i].isOnline == 0) {
				/* 5次心跳包都没有回应 则将用户的文件描述符移出从反应堆（用户下线） */
				int whichsub = msg.sex ? epollfd2 : epollfd3;
				epoll_ctl(whichsub, EPOLL_CTL_DEL, users[i].fd, NULL);
				close(users[i].fd);
				DBG(RED"<Server Heartbeat>"NONE" %s is removed from sub reactor %d , because of heart beat no response.\n", users[i].fd, whichsub);
			}
		}
	}
}

void broadcast(struct wechat_msg *msg) {
	for (int i = 0; i < CUR_USER_NUMBER; ++i) {
		if (users[i].isOnline && strcmp(users[i].name, msg->from)) {
			DBG(RED"<Sub Reactor-broadcast>"NONE" : %d shall be broadcasted a message <%s>.\n", users[i].fd, msg->content);
			send(users[i].fd, (void *)msg, sizeof(*msg), 0);
		}
	}
}

int secret(struct wechat_msg *msg) {
	for (int i = 0; i < CUR_USER_NUMBER; ++i) {
		if (strcmp(users[i].name, msg->to) == 0) {
			if (users[i].isOnline) {
				DBG(RED"<Sub Reactor-secret>"NONE" : %d shall be secret a message <%s>.\n", users[i].fd, msg->content);
				send(users[i].fd, (void *)msg, sizeof(*msg), 0);
				return 1; 
			}
		}
	}
	return 0;
}



