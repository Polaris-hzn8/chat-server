/*************************************************************************
	> File Name: preread.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 04:12:40 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/read.h"

// TODO 全局变量conf_value在函数中被使用，可能需要确保多次调用函数时不会出现竞态条件

// 读取并获得conf文件中的配置信息 存储在全局变量conf_value中
char *get_conf_value(const char *config_file_path, const char *key) {
    FILE *fp;
    char *line = NULL;   //获取的行
    char *idx = NULL;    //找到的目标下标指针
    //len用于存储已分配的行缓冲区的长度
    //nread存储getline函数成功读取的行的字符数 可以用于检查当前行是否空
    long unsigned int len = 0, nread = 0;
    bzero(conf_value, sizeof(conf_value));

    // 1.打开配置文件
    // fopne等系统调用如果出错errno会自动被设置 选择直接return NULL而不是perror 让报错信息在调用该函数层处被输出
    if ((fp = fopen(config_file_path, "r")) == NULL) return NULL;
    
	// 2.进行目标key的查找 逐行读取配置文件 将行保存到line变量中（getline会自动分配足够的内存来存储行）
    // ssize_t getline(chat **lineptr, size_t *n, FILE *stream);
    // getline() != -1 表示没有出错 or 没有触及 EOF
    while ((nread = getline(&line, &len, fp)) != -1) {
        // 2-1.在当前行中寻找关键字key
        idx = strstr(line, key);
        if (idx == NULL) continue;
        // 2-2.当前所在位置 sub == line 并且 key之后紧跟等号 则为需要获取配置值的情况
        if (idx == line && line[strlen(key)] == '=') {
            // 进行配置文件值的拷贝
            strcpy(conf_value, line + strlen(key) + 1);
            // 处理行尾换行符回车\n
            if (conf_value[strlen(conf_value) - 1] == '\n')
                conf_value[strlen(conf_value) - 1] = '\0';
        }
    }
    fclose(fp);
    free(line);//man提示需要手动free
    if (!strlen(conf_value)) return NULL;
    return conf_value;
}

