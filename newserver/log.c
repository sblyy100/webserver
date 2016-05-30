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
void log_debug(unsigned int level, const char *fmt, ...)
{
	FILE *file;
	unsigned char logstr[1024] = {0};
    UINT32 uiLen = 0;
    pthread_mutex_lock(&(dlog->log_lock));
    if (dlog->OPCODE != LOG_TO_STDERR)
    {
        file = fopen(dlog->filename, "a+");
        if (file == NULL)
        {
            perror("open file err");
            return;
        }
    }
    else
    {
        file = stderr;
    }
    
	va_list var;
	
	va_start(var,fmt);
    
	vsprintf(logstr ,fmt, var);
	va_end(var);
    
    fprintf(file,"%s %s %5s (%s:%d) %s ",__DATE__,__TIME__,uclogLevel[level],__FILE__,__LINE__,__FUNCTION__);
    fprintf(file,"%s\r\n", logstr);
    fclose(file);
	pthread_mutex_unlock(&(dlog->log_lock));
}
UINT32 init_log(){
	UINT32 uiRet;

    dlog = (log_t *)malloc(sizeof(log_t));
	if(!dlog){
		perror("init log error!");
		return ERR;
	}
    #if 0
	if((srv)->logdir){
		memcpy(dlog->filename, srv->logdir, strlen(srv->logdir));
	}
	else{
		memcpy(dlog->filename, LOGFILE , strlen(srv->logdir));
	}
    #endif
	memcpy(dlog->filename, LOGFILE , strlen(LOGFILE));
	//log->log_lock=PTHREAD_MUTEX_INITALIZER;
	if(pthread_mutex_init(&(dlog->log_lock),NULL)<0){
		perror("init log lock error");
		return ERR;
	}
	perror("init log lock ok");
	#ifdef LOG_DEBUG
		dlog->OPCODE = LOG_TO_STDERR;
	#endif
	
	return OK;

}
void fini_log(){
	if(dlog){
		
		if(pthread_mutex_destroy(&(dlog->log_lock))<0)
			perror("log lock destroy failed");
		}
		free(dlog);
		dlog=NULL;
		printf("free log...\n");
}
#ifdef LOG_DEBUG
int main(){
    init_conf(&srv);
    perror("init server ok");
    init_log(&srv);

	log_debug(LOG_LEVEL_DEBUG,"%s","test log\n");
    fini_log();
}
#endif
