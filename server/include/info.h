/*************************************************************************
	> File Name: info.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 04:21:46 PM CST
 ************************************************************************/

#ifndef _INFO_H
#define _INFO_H

#define M_MAXEVENTS 5
#define S_MAXEVENTS 5
#define MAXUSERS 1024

// 信令 比特掩码
#define WECHAT_ACK 0x01			//ACK
#define WECHAT_NAK 0x02			//NAK
#define WECHAT_FIN 0x04			//FIN
#define WECHAT_HEART 0x08		//心跳

#define WECHAT_SIGNUP 0x10		//注册
#define WECHAT_SIGNIN 0x20		//登录
#define WECHAT_SIGNOUT 0x40		//退出

#define WECHAT_SYS 0x80			//系统消息
#define WECHAT_WALL 0x100		//广播消息
#define WECHAT_MSG 0x200		//私发消息
#define WECHAT_ACT 0x400		//查询在线活跃人数

struct wechat_user {
	char name[50];				//用户名
	char passwd[20];			//用户密码
	int sex;					//用户性别
	int isOnline;				//用户在线状态
	int fd;						//用户在哪个socket连接上
};

struct wechat_msg {
	int cid;					//用户信令
	char from[50];				//消息发送方
	char to[50];				//消息接收方
	char content[1024];			//消息内容
	int sex;					//发送方用户的性别（仅用于临时的负载均衡）
	int actUser;				//用户选择查询当前系统在线人数
};

int actUser;
struct wechat_user *users;
int epollfd1, epollfd2, epollfd3;

#endif
