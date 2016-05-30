#ifndef _LOG_H
#define _LOG_H
#include "base.h"
#include <stdarg.h>
#include <pthread.h>
#include <stdio.h>
#include "config.h"

#define LOG_2_FILE 0
#define LOG_2_STDERR 1
typedef struct {
	pthread_mutex_t log_lock;
	unsigned int OPCODE;
    unsigned char filename[128];
}log_t;
enum tagOpCode
{
    LOG_TO_FILE,
    LOG_TO_STDERR,
};

enum tagLogLevel
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERR,
    LOG_LEVEL_MAX,
};


//#define log_debug(level,fmt,args...) mylog(level,fmt,##args)
//#define log_debug(level,fmt,args...) mylog(level,fmt,args...)



void log_debug(unsigned int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

UINT32 init_log();
void fini_log();



#endif
