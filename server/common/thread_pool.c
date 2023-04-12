/*************************************************************************
	> File Name: thread_pool.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 28 Mar 2023 04:18:13 PM CST
 ************************************************************************/

#include "head.h"
#include "thread_pool.h"

void task_queue_init(struct task_queue *taskQueue, int size) {
	taskQueue->size = size;
	taskQueue->count = taskQueue->head = taskQueue->tail = 0;
	taskQueue->data = calloc(size, sizeof(void *));
	pthread_mutex_init(&taskQueue->mutex, NULL);
	pthread_cond_init(&taskQueue->cond, NULL);
}

void task_queue_push(struct task_queue *taskQueue, void *data) {
	/* 为了保证线程的安全性 所有对临界区的操作都需要加锁 */
	pthread_mutex_lock(&taskQueue->mutex);
	if (taskQueue->count == taskQueue->size) {
		DBG(YELLOW"<push> : taskQueue is full\n"NONE);
		pthread_mutex_unlock(&taskQueue->mutex);
		return;
	}
	taskQueue->data[taskQueue->tail] = data;
	DBG(GREEN"<push> : data is pushed!\n"NONE);
	taskQueue->tail++;
	taskQueue->count++;
	/* 考虑循环队列 */
	if (taskQueue->tail == taskQueue->size) {
		DBG(YELLOW"<push> : taskQueue tail reach end!\n"NONE);
		taskQueue->tail = 0;
	}
	pthread_cond_signal(&taskQueue->cond);//信号量
	pthread_mutex_unlock(&taskQueue->mutex);
	return;
}

void *task_queue_pop(struct task_queue *taskQueue) {
	pthread_mutex_lock(&taskQueue->mutex);
	//使用while循环而不是if语句 处理惊群效应
	while (taskQueue->count == 0) {
		/* 当任务队列中没有任务时 线程选择等待而不是直接return
		1.如果让线程直接返回则意味着一会还需要让线程轮训回来（轮询时间有要求）
		2.轮询时间太短则消耗CPU
		3.轮询时间太长则相应能力下降
		 */
		pthread_cond_wait(&taskQueue->cond, &taskQueue->mutex);//cond与mutex同时使用
	}
	void *data = taskQueue->data[taskQueue->head];
	DBG(RED"<pop> : data is poped!\n"NONE);
	taskQueue->count--;
	taskQueue->head++;
	/* 考虑循环队列 */
	if (taskQueue->head == taskQueue->size) {
		DBG(YELLOW"<pop> : taskQueue head reach end!\n"NONE);
		taskQueue->head = 0;
	}
	pthread_mutex_unlock(&taskQueue->mutex);
	return data;
}




