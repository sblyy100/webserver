#ifndef NETWORK_H
#define NETWORK_H
#define MAX_CON 4096
#include "base.h"
#include <pthread.h>
#include <stdlib.h>
#if 0
typedef struct connection_s{
	uint32_t epfd;
	uint32_t client_fd;
	uint32_t c_or_s;
	uint32_t clientip;
	uint32_t serverip
	uint16_t clientport;
	uint16_t serverport;
	//uint32_t timeout;
	//uint32_t unset;
}connection_t;
#endif
/*between worker and listener*/
typedef struct tag_con_stack {
	pthread_mutex_t stack_lock;
	int top;
	int con[MAX_CON];
}CON_STACK_t;
//extern struct con_stack *connections;
int stack_init(CON_STACK_t *S);
int stack_fini(CON_STACK_t *S);
int con_pop(CON_STACK_t *S, UINT32 *fd);
int con_pop_batch(CON_STACK_t *S, UINT32 *fd, UINT32 *n);

int con_push(CON_STACK_t *S,int fd);
#endif
