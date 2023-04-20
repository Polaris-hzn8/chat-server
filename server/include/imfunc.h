/*************************************************************************
	> File Name: imfunc.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 04:39:18 PM CST
 ************************************************************************/

#ifndef _IMFUNC_H
#define _IMFUNC_H

#include "head.h"
#include "info.h"

void heart_beat(int signum);
void broadcast(struct wechat_msg *msg);

#endif
