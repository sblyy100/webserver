#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "config.h"
#define LOGFILE "/var/log/server.log"
int log_to;
log_t *dlog = NULL;

char uclogLevel[LOG_LEVEL_MAX][128] = 
{
    "DEBUG",
    "INFO",
    "WARN",
    "ERR",
};
void mylog(unsigned int level, char *fmt,...)
{
	//FILE *fn;
	unsigned char logstr[1024] = {0};
	va_list var;
	sprintf(logstr,"%s %s %5s (%s:%d) %s:%s",__DATE__,__TIME__,uclogLevel[level],__FILE__,__LINE__,__FUNCTION__);
	va_start(var,fmt);
	sprintf(logstr,fmt,var);
	va_end(var);
    pthread_mutex_lock(&(dlog->log_lock));
		if (dlog->log)
			fprintf(dlog->log,logstr);
	pthread_mutex_unlock(&(dlog->log_lock));
}
log_t* init_log(struct server_conf *srv){
	FILE *fn;
	char *logdir;
	log_t *log;
	if(!srv){
		perror("server conf null");
	}
	if((srv)->logdir){
		logdir=(srv)->logdir;
		//printf("log file is %s\n",logdir);
	}
	else{
		logdir=LOGFILE;
		//printf("log file is %s\n",logdir);
	}
	log=(log_t *)malloc(sizeof(log_t));
	printf("init log start\n");
	if(!log){
		perror("init log error!");
		exit(-1);
	}
	perror("init log ok");
	//log->log_lock=PTHREAD_MUTEX_INITALIZER;
	if(pthread_mutex_init(&(log->log_lock),NULL)<0){
		perror("init log lock error");
		exit(-1);
	}
	perror("init log lock ok");
	#ifdef LOG_DEBUG
		log_to=LOG_2_STDERR;
	#endif
	if(log_to==LOG_2_STDERR)
		log->log=stderr;
	else{
		fn=fopen(logdir,"a");
		//printf("fd is %d\n",fd);
		if(fn<0){
			perror("open log file error\n");
			exit(0);
		}
		log->log=fn;
	}
	return log;

}
void fini_log(log_t *log){
	if(log){
		if(log->log){
			close(log->log);
		}
		if(pthread_mutex_destroy(&(log->log_lock))<0)
			perror("log lock destroy failed");
		}
		free(log);
		log=NULL;
		printf("free log...\n");
}
#ifdef LOG_DEBUG
int main(){
    init_conf(&srv);
    perror("init server ok");
    dlog=init_log(&srv);

	log_debug(LOG_LEVEL_DEBUG,"%s","test log\n");
    fini_log(dlog);
}
#endif
