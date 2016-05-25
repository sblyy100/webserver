#ifndef _LOG_H
#define _LOG_H
#include <stdarg.h>
#include <pthread.h>
#include <stdio.h>
#include "config.h"
extern int log_to;//
#define LOG_2_FILE 0
#define LOG_2_STDERR 1
typedef struct {
	pthread_mutex_t log_lock;
	FILE *log;
}log_t;

void mylog(char *fmt,...);
log_t* init_log(struct server_conf *);
void fini_log(log_t *);

#define log_debug(level, fmt,args...) mylog(level,fmt,##args)

#endif
