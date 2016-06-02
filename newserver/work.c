#include "work.h"
#include "network.h"
#include "listen.h"
#include "log.h"
#include "response.h"
#include "unix_sock.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h> //struct sockaddr_in required
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "http.h"
//#include <arpa/inet.h>
#define REQ_MAX 1024
extern CON_STACK_t *g_sock_stack;
extern FD_SESSION_t *g_fd_session_table; 
extern SERVER_CONF_T srv;

static __thread INT32 uiThread_epfd = 0;

void shut_down(void){
    exit(1);
}
void work_clean(){
}
void work_thread(){
    char logstr[1024];
    int epfd;//epoll fd
    int newfd[128];//pop the accept fd
    int nfds;//epoll wait return fd
    int i, j,n=128;
    UINT32 uiRet;
    
    struct epoll_event *events;
    //pthread_cleanup_push(&work_clean,NULL);
    events=(struct epoll_event*)malloc(MAX_CON*sizeof(struct epoll_event));
    if(!events){
        log_debug(LOG_LEVEL_DEBUG,"events malloc failed");
        exit(-1);
    }
    uiThread_epfd = epoll_create(256);
    if(-1 == uiThread_epfd){
        log_debug(LOG_LEVEL_ERR,"events malloc failed");
        exit(-1);
    }
	while(1){
	    n = 128;
        //pop array
		uiRet = con_pop_batch(g_sock_stack, newfd, &n);
        #if 0
        if(uiRet != OK || n <=0){
			//log_debug(LOG_LEVEL_DEBUG,"connection pop err,continue");
            sleep(1);
			continue;
		}
        #endif
        for (i = 0; i < n; i++)
        {
            fd_session_init(newfd[i]);
            
        }
		
		if((nfds=epoll_wait(uiThread_epfd,events,MAX_CON,1000))>0){
			for(j=0;j<nfds;j++)
				process_events(events+j);
		}

	}
	close(uiThread_epfd);
	//pthread_cleanup_pop(1);
}
//init_clientfd(fd)
int process_events(struct epoll_event* ev){
    INT32 fd = ev->data.fd;
    FD_SESSION_t *pstFd_session = g_fd_session_table + fd;

	log_debug(LOG_LEVEL_DEBUG, "events: 0x%x",ev->events);
	if(ev->events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)){
		log_debug(LOG_LEVEL_DEBUG, "remove fd because EPOLLHUP , %d",fd);
        
        fd_session_close(fd);
		return OK;
	}
   
	if(ev->events & EPOLLIN)
    {
		if (work_process(fd) != OK)
        {
            log_debug(LOG_LEVEL_DEBUG, "work_process failed %d",fd);
        }
        pstFd_session->event_set = SESSION_IO_READ;
        fd_set_events(fd);
        //fd_session_close(ev->data.fd);
		return OK;
	}
}

int work_process(int fd)
{
    char *request;
	http_request_line_t *request_line;
	http_response_t response;
	response.len=0;
	response.data=NULL;
    INT32 ret = ERR;
    request=(char *)calloc(REQ_MAX,sizeof(char));
	if(!request){
		log_debug(LOG_LEVEL_DEBUG,"request malloc failed");
		goto end;
	}
	log_debug(LOG_LEVEL_DEBUG,"malloc for request buffer ok!");
	request_line=(http_request_line_t*)calloc(1,sizeof(http_request_line_t));
	if(!request_line){
		log_debug(LOG_LEVEL_DEBUG,"malloc for request decode buf error");
		goto end;
	}
	log_debug(LOG_LEVEL_DEBUG,"malloc for request line buffer ok");
	if(recv(fd,request,REQ_MAX,0)>0){
		log_debug(LOG_LEVEL_DEBUG,"receive  from fd %d",fd);
		request_decode(fd,request,request_line);
		http_response_create(&srv,request_line,&response);
		ret = OK;
	}
    else{ 
        fd_session_close(fd);
    }
end:
    if(request)
    {
		free(request);
		request=NULL;
	}
	if(request_line){
		free(request_line);
		request_line=NULL;
	}
    return ret;
}
static int set_nonblock(int fd){
    int flag;
    flag=fcntl(fd,F_GETFL);
    if(flag&O_NONBLOCK)
        return 0;
    else
        fcntl(fd,F_SETFL,O_NONBLOCK);
    return 0;
}

int fd_session_init(int fd)
{
    struct epoll_event ev;
    FD_SESSION_t *pstFd_session;
    pstFd_session = g_fd_session_table + fd;
    pstFd_session->epfd = uiThread_epfd;
    //pstFd_session->event_set
    log_debug(LOG_LEVEL_DEBUG,"fd_session_init %u", fd);
    set_nonblock(fd);
    socket_opt_set(fd, OPT_TCP_CORK, 1);
    pstFd_session->in_queue = 0;
    pstFd_session->event_set = SESSION_IO_READ;
    fd_set_events(fd);
    
    return OK;
}
int fd_session_close(int fd)
{
    struct epoll_event ev;
    FD_SESSION_t *pstFd_session;
    pstFd_session = g_fd_session_table + fd;

    pstFd_session->event_set= SESSION_IO_IDLE;
    fd_set_events(fd);
    memset(pstFd_session,0,sizeof(FD_SESSION_t));
    log_debug(LOG_LEVEL_DEBUG,"fd_session_delete %u", fd);
    while (0 != close(fd)) 
    {
        log_debug(LOG_LEVEL_DEBUG,"close fd %d: %m", fd);
        switch (errno) {
        case EINTR: //The close() call was interrupted by a signal; see signal(7).
            continue;
        case EBADF: //fd isn't a valid open file descriptor.
            return ERR;
        default:
            usleep(100);
        }
    }
    return OK;
}
int fd_set_events(int fd)
{
    struct epoll_event ev;
    FD_SESSION_t *pstFd_session = g_fd_session_table + fd;
    UINT32 status = pstFd_session->event_set;
    INT32 epoll_type; 
    INT32 ret;
    if (status == SESSION_IO_IDLE)
    {
        if (pstFd_session->in_queue == 0)
        {
            return OK;
        }
        //delete
        pstFd_session->event_status = 0;
        pstFd_session->in_queue = 0;
        pstFd_session->event_set = 0;
        epoll_ctl(uiThread_epfd,EPOLL_CTL_DEL,fd,NULL);
        log_debug(LOG_LEVEL_DEBUG,"fd_set_events delete fd: %u ", fd);
        return OK;
    }
    if (status == pstFd_session->event_status)
        return OK;
    if (pstFd_session->in_queue == 0)
    {
        epoll_type = EPOLL_CTL_ADD;
    }
    else
    {
        epoll_type = EPOLL_CTL_MOD;
    }
    if (status & SESSION_IO_READ)
    {
        ev.events |= EPOLLIN;
    }
    else if (status & SESSION_IO_WRITE)
    {
        ev.events |= EPOLLOUT;
    }
    if (ev.events) 
        ev.events |= EPOLLPRI | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
    ev.data.fd = fd;
    ret = epoll_ctl(pstFd_session->epfd, epoll_type, fd, &ev);
    if (ret != OK)
    {
        log_debug(LOG_LEVEL_DEBUG,"fd_set_events set fd: %u --> %x failed", fd, ev.events);
        return ERR;
    }
    pstFd_session->event_status = status;
    pstFd_session->in_queue = 1;
    return OK;
}

