#ifndef __RARCH_LOGX_H
#define __RARCH_LOGX_H

#include <stdio.h>

#define LOGX(format, ...) printf("%s:%d %s - " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#endif