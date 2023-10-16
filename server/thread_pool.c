/*************************************************************************
	> File Name: thread_pool.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 28 Mar 2023 04:18:13 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/thread_pool.h"

// 初始化任务队列 传入队列的指针 taskQueue 和队列的大小 size
void task_queue_init(struct task_queue *taskQueue, int size) {
	// 设置队列大小、计数、头尾指针等参数 并分配一定大小的内存用于存储队列数据
	taskQueue->size = size;
	taskQueue->count = taskQueue->head = taskQueue->tail = 0;
	taskQueue->data = calloc(size, sizeof(void *));
	// 初始化互斥锁 mutex 和条件变量 cond 用于线程同步
	pthread_mutex_init(&taskQueue->mutex, NULL);
	pthread_cond_init(&taskQueue->cond, NULL);
}

// 将数据推送到任务队列中
void task_queue_push(struct task_queue *taskQueue, void *data) {
	// 为了保证线程的安全性 所有对临界区的操作都需要加锁
	pthread_mutex_lock(&taskQueue->mutex);					//使用互斥锁 mutex 来保护对队列的并发访问
	if (taskQueue->count == taskQueue->size) {				//如果队列已满，函数输出调试信息并释放锁，否则将数据存储在队列的尾部
		DBG(YELLOW"<push> : taskQueue is full\n"NONE);
		pthread_mutex_unlock(&taskQueue->mutex);
		return;
	}
	taskQueue->data[taskQueue->tail] = data;
	DBG(GREEN"<push> : data is pushed!\n"NONE);				//更新队列的尾指针、计数，并在必要时考虑循环队列
	taskQueue->tail++;
	taskQueue->count++;
	// 考虑循环队列
	if (taskQueue->tail == taskQueue->size) {
		DBG(YELLOW"<push> : taskQueue tail reach end!\n"NONE);
		taskQueue->tail = 0;
	}
	pthread_cond_signal(&taskQueue->cond);					//发送条件变量信号 以通知可能在等待任务的线程
	pthread_mutex_unlock(&taskQueue->mutex);				//释放互斥锁
	return;
}

// 从任务队列中弹出数据 传入队列指针 taskQueue
void *task_queue_pop(struct task_queue *taskQueue) {
	pthread_mutex_lock(&taskQueue->mutex);					//使用互斥锁 mutex 来保护对队列的并发访问
	//使用while循环而不是if语句 处理惊群效应
	while (taskQueue->count == 0) {
		/* 当任务队列中没有任务时 线程选择等待而不是直接return
		1.如果让线程直接返回则意味着一会还需要让线程轮训回来（轮询时间有要求）
		2.轮询时间太短则消耗CPU
		3.轮询时间太长则相应能力下降
		 */
		//cond与mutex同时使用
		pthread_cond_wait(&taskQueue->cond, &taskQueue->mutex);
	}
	void *data = taskQueue->data[taskQueue->head];			//当队列不为空时，从队列的头部获取数据，更新队列头指针、计数，并在必要时考虑循环队列
	DBG(RED"<pop> : data is poped!\n"NONE);
	taskQueue->count--;
	taskQueue->head++;
	/* 考虑循环队列 */
	if (taskQueue->head == taskQueue->size) {
		DBG(YELLOW"<pop> : taskQueue head reach end!\n"NONE);
		taskQueue->head = 0;
	}
	pthread_mutex_unlock(&taskQueue->mutex);				//最后释放互斥锁，并返回弹出的数据指针
	return data;
}




