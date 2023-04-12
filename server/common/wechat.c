/*************************************************************************
	> File Name: wechat.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Sat 08 Apr 2023 11:32:52 PM CST
 ************************************************************************/

#include "head.h"
#include "wechat.h"

void *sub_reactor(void *arg) {
	int subefd = *(int *)arg;
	while (1) {
		DBG(L_RED"<Sub Reactor>"NONE" : in sub reactor %d.\n", subefd);
		sleep(3000);
	}
}
