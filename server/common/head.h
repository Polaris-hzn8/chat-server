/*************************************************************************
	> File Name: head.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Fri 10 Feb 2023 06:59:31 AM CST
 ************************************************************************/

#ifndef _HEAD_H
#define _HEAD_H

#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>

#include<grp.h>
#include<pwd.h>
#include<time.h>

#include<poll.h>

#include<sys/wait.h>

#include<sys/file.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<pthread.h>

#include<sys/sem.h>
#include<sys/msg.h>

#include<pthread.h>

#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<signal.h>

#include <sys/time.h>

#include <sys/epoll.h>

#include "color.h"

//开发过程中的 通过宏定义来定义调试信息
#ifdef _D
#define DBG(fmt, arg...) printf(fmt, ##arg)//fmt格式化字符串、args可变参数
#else
#define DBG(fmt, arg...)
#endif

#endif
