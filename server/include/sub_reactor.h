/*************************************************************************
	> File Name: wechat.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Sat 08 Apr 2023 11:40:32 PM CST
 ************************************************************************/

#ifndef _WECHAT_H
#define _WECHAT_H

int add_to_subreactor(int epollfd, int fd);
void *sub_reactor(void *arg);

#endif
