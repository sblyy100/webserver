#ifndef NETWORK_H
#define NETWORK_H
#define MAX_CON 4096
#include "base.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct connection_s{
    UINT32 epfd;
    UINT32 fd;
    UINT32 fd_status:8;
    UINT32 event_status:4;
    UINT32 event_set:4;
    UINT32 in_queue:1; //是否已经在epoll队列中
    UINT32 resv;
}FD_SESSION_t;

/*FD 状态*/
enum {
    SESSION_FD_CLOSED     = 0,      
    SESSION_FD_CONNECTING = (1<<0),
    SESSION_FD_CONNECTED  = (1<<1),
    SESSION_FD_IOBUSY     = (1<<2),
    SESSION_FD_HUPERR     = (1<<3),
    SESSION_FD_CLOSING    = (1<<4),
    SESSION_FD_SSL_HANDSHAKING = (1 << 5)
};
/*epoll 状态*/
enum {
    SESSION_IO_IDLE   = 0,      
    SESSION_IO_READ =  1<<0,
    SESSION_IO_WRITE = (1<<1),
    SESSION_IO_CLOSE = (1u<<2), 
};
/*TCP OPTION*/
enum
{
    OPT_TCPRST = (1<<0),
    OPT_KEEPALIVE = (1<<1),
    OPT_REUSEADDR = (1<<2),
    OPT_SNDBUF = (1<<3),
    OPT_RCVBUF = (1<<4),
    OPT_IP_TPROXY = (1<<5),
    OPT_TCP_NODELAY = (1<<6),
    OPT_TCP_CORK = (1<<7),
    OPT_TCP_DEFER_ACCEPT = (1<<8),
    OPT_TCP_SYNCNT = (1<<9),
};
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
int socket_opt_set(int fd, int optname, int optval);



#endif
