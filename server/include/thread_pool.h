/*************************************************************************
	> File Name: thread_pool.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 28 Mar 2023 04:18:21 PM CST
 ************************************************************************/

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include "head.h"

struct task_queue {
	int head, tail;
	int size;//队列容量
	int count;//已经入队的元素
	void **data;//模拟任务
	pthread_mutex_t mutex;//需要互斥锁加锁
	pthread_cond_t cond;//信号量
};

void task_queue_init(struct task_queue *taskQueue, int size);//队列初始化
void task_queue_push(struct task_queue *taskQueue, void *data);//入队
void *task_queue_pop(struct task_queue *taskQueue);//出队

#endif
