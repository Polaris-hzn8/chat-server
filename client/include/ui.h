/*************************************************************************
	> File Name: ui.h
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 05:01:54 PM CST
 ************************************************************************/

#ifndef _UI_H
#define _UI_H

#include <ncurses.h>
#include <locale.h>

#define MSG_WIDTH 80
#define MSG_HEIGHT 20
#define INFO_WIDTH 10

WINDOW *msg_win, *sub_msg_win, *info_win, *sub_info_win, *input_win, *sub_input_win;

void init_ui();
void gotoxy_puts(int x, int y, char* s);//走到xy位置放置1个字符串
void gotoxy_putc(int x, int y, char c);//走到xy位置放置1个字符
void gotoxy(int x, int y);//走到xy位置放置

void w_gotoxy_puts(WINDOW *win, int x, int y, char* s);//走到窗口的xy位置放置1个字符串
void w_gotoxy_putc(WINDOW *win, int x, int y, char c);//走到窗口的xy位置放置1个字符
void destroy_win(WINDOW *win);//走到窗口的xy位置放置

WINDOW *create_newwin(int width, int height, int startx, int starty);//创建一个新的窗口
void destroy_win(WINDOW *win);//销毁一个窗口

void show_msg(struct ChatMsg *msg);//显示消息

#endif
