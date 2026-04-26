#ifndef CONF_H
#define CONF_H

// 取消注释以下行以启用调试输出
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)
#endif

#endif // CONF_H
