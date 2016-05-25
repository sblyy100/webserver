#ifndef _LOG_H
#define _LOG_H
#include <stdarg.h>
#include <pthread.h>
#include <stdio.h>
#include "config.h"

#define LOG_2_FILE 0
#define LOG_2_STDERR 1
typedef struct {
	pthread_mutex_t log_lock;
	FILE *log;
}log_t;

enum tagLogLevel
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERR,
    LOG_LEVEL_MAX,
};


#define log_debug(level, fmt,args...) mylog(level,fmt,##args)

void mylog(unsigned int level, char *fmt,...);
log_t* init_log(struct server_conf *);
void fini_log(log_t *);



#endif
