/*************************************************************************
	> File Name: wechat.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Sat 08 Apr 2023 11:40:32 PM CST
 ************************************************************************/

#ifndef _WECHAT_H
#define _WECHAT_H

/* 比特掩码 */
#define WECHAT_ACK 0x01
#define WECHAT_NAK 0x02
#define WECHAT_FIN 0x04
#define WECHAT_HEART 0x08

#define WECHAT_SIGNUP 0x10//注册
#define WECHAT_SIGNIN 0x20//登录
#define WECHAT_SIGNOUT 0x40//退出

#define WECHAT_SYS 0x80//系统消息
#define WECHAT_WALL 0x100//广播消息
#define WECHAT_MSG 0x200//私发消息

#define M_MAXEVENTS 5
#define S_MAXEVENTS 5
#define MAXUSERS 1024

struct wechat_user {
	char name[50];//用户名
	char passwd[20];//用户密码
	int sex;
	int isOnline;
	int fd;//用户在哪个连接上
};

struct wechat_msg {
	int type;
	char from[50];
	char to[50];
	char msg[1024];
	int sex;
};

#endif
