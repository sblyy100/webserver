#include "network.h"
#include "log.h"
#include <stdlib.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>


FD_SESSION_t *g_fd_session_table = NULL; 
CON_STACK_t *g_sock_stack = NULL;
int stack_init(CON_STACK_t *S){
	int ret;
    
    #if 0
	stack=(struct con_stack*)malloc(sizeof(struct con_stack));
	if(stack == NULL)
    {
        log_debug(LOG_LEVEL_DEBUG,"malloc for connection stack error");
		exit(-1);
    }   
    #endif
	ret=pthread_mutex_init(&(S->stack_lock),NULL);
	if(ret<0){
		log_debug(LOG_LEVEL_DEBUG,"stack lock init err: %d %m", ret);
		return 0;
	}
	S->top=-1;
	
	memset(S->con,MAX_CON,0);
    //*S = stack;
	return OK;
}
int stack_fini(CON_STACK_t *S){
	pthread_mutex_destroy(&S->stack_lock);
	//free(S);
	//S=NULL;
	return 1;
}
int con_pop(CON_STACK_t *S, UINT32 *fd){

    if(NULL==S){
        //log_debug(LOG_LEVEL_DEBUG,"connctions stack NULL!");
        return ERR;
    }

    if(S->top<0){
        //log_debug(LOG_LEVEL_DEBUG,"connctions top < 0!");
        return ERR;
    }
        
	if (pthread_mutex_trylock(&(S->stack_lock)) != 0)
    {
        return ERR;
    }   
	//pthread_mutex_lock(&(S->stack_lock));
	*fd = S->con[S->top--];
	log_debug(LOG_LEVEL_DEBUG,"pop a socket %d", *fd);
	pthread_mutex_unlock(&(S->stack_lock));
    return OK;
}
int con_pop_batch(CON_STACK_t *S, UINT32 *fd, UINT32 *n){
    UINT32 i;
    
    if (n == NULL || *n == 0)
    {
        return ERR;
    }
    if(NULL==S){
        return ERR;
    }
    if(S->top<0){
        return ERR;
    }
        
	if (pthread_mutex_trylock(&(S->stack_lock)) != 0)
    {
        return ERR;
    }   
	//pthread_mutex_lock(&(S->stack_lock));
	for(i = 0; i < *n; i++)
    {        
        if (S->top < 0)
        {
            break;
        }
        fd[i] = S->con[S->top--];
        log_debug(LOG_LEVEL_DEBUG,"pop  %u", fd[i]);
    }
    *n = i;
    log_debug(LOG_LEVEL_DEBUG,"pop batch socket %u", *n);
    
	pthread_mutex_unlock(&(S->stack_lock));
    return OK;
}

int con_push(CON_STACK_t *S,int fd){
	char logstr[1024];
	if(S->top<MAX_CON){
		pthread_mutex_lock(&(S->stack_lock));
		S->con[++S->top]=fd;
		log_debug(LOG_LEVEL_DEBUG,"current connection top is %d",S->top);
		pthread_mutex_unlock(&(S->stack_lock));
		return 1;
	}
	return -1;
}

int socket_opt_set(int fd, int optname, int optval)
{
    int enable;
    enable = optval ? 1:0;
    switch(optname)
    {
        case OPT_TCP_CORK:
            if (setsockopt(fd, IPPROTO_TCP, TCP_CORK, (char *) &enable, sizeof(enable)) < 0) 
            {
                log_debug(LOG_LEVEL_DEBUG,"set opt TCP_CORK failed,%d",fd);
                return ERR;
            }
            break;
        default:
            break;
    }
    return OK;
}
