#ifndef __LOG_PRINT_H
#define __LOG_PRINT_H

#include "debug.h"

#ifndef LOG_PRINT_ENABLE
#define LOG_PRINT_ENABLE 1
#endif

#if LOG_PRINT_ENABLE
#define LOG_PRINT(...) PRINT(__VA_ARGS__)
#else
#define LOG_PRINT(...) ((void)0)
#endif

#endif /* __LOG_PRINT_H */
