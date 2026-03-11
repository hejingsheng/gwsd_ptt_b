//
// Created by hejingsheng on 2024/12/4.
//

#ifndef GWSD_APP_GWLOG_H
#define GWSD_APP_GWLOG_H

#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define GW_LOG_LEVEL_VERBOSE    0
#define GW_LOG_LEVEL_DEBUG      1
#define GW_LOG_LEVEL_INFO       2
#define GW_LOG_LEVEL_WARN       3
#define GW_LOG_LEVEL_ERROR      4
#define GW_LOG_LEVEL_NEVER      5

void print_log(const char *fmt, ...);

extern int gwlog_level;
#define GWLOG_PRINT(level, fmt, ...)       \
    do                                     \
    {                                      \
        if(level >= gwlog_level)             \
        {                                  \
            print_log(fmt, ##__VA_ARGS__); \
        }                                  \
    }while(0);                             \

#ifdef  __cplusplus
}
#endif
#endif //GWSD_APP_GWLOG_H
