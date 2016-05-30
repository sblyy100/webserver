#include "network.h"
#include "log.h"
#include <stdlib.h>
#include <pthread.h>
int stack_init(struct con_stack *S){
	int ret;
	//S=(struct con_stack*)malloc(sizeof(struct con_stack));
	//if()
	ret=pthread_mutex_init(&(S->stack_lock),NULL);
	if(ret<0){
		log_debug(LOG_LEVEL_DEBUG,"stack lock init err: %d %m", ret);
		return 0;
	}
	S->top=-1;
	
	memset(S->con,MAX_CON,0);
	return 1;
}
int stack_fini(struct con_stack *S){
	pthread_mutex_destroy(&S->stack_lock);
	free(S);
	S=NULL;
	return 1;
}
int con_pop(struct con_stack *S, UINT32 *fd){

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
int con_push(struct con_stack *S,int fd){
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
