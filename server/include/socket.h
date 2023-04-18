/*************************************************************************
	> File Name: common.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Thu 30 Mar 2023 04:23:41 PM CST
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H

int socket_create(int port);
int socket_connect(const char *ip, int port);

int make_nonblock(int fd);
int make_block(int fd);

#endif
