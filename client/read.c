/*************************************************************************
	> File Name: preread.c
	> Author: luochenhao
	> Mail: 3453851623@qq.com
	> Created Time: Tue 18 Apr 2023 04:12:40 PM CST
 ************************************************************************/

#include "include/head.h"
#include "include/read.h"

//读取并获得conf文件中的配置信息
char *get_conf_value(const char *filename, const char *key) {
    FILE *fp;
    char *line = NULL;//获取的行
    char *sub = NULL;//找到的目标下标指针
    ssize_t len = 0, nread = 0;
    bzero(ans, sizeof(ans));
    //1.打开配置文件
    if ((fp = fopen(filename, "r")) == NULL) return NULL;
    
	//2.逐行读取文件中的信息 进行目标key的查找
    while ((nread = getline(&line, &len, fp)) != -1) {
        if ((sub = strstr(line, key)) == NULL) continue;
        if (sub == line && line[strlen(key)] == '=') {
            /* sub == line && line[strlen(key)]为等于号 */
            strcpy(ans, line + strlen(key) + 1);
            if (ans[strlen(ans) - 1] == '\n') {
                ans[strlen(ans) - 1] = '\0';//行尾的换行符处理
            }
        }
    }
    fclose(fp);
    free(line);
    if (!strlen(ans)) return NULL;
    return ans;
}

