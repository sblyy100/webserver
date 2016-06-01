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
#include "http.h"
//#include <arpa/inet.h>
#define REQ_MAX 1024
extern CON_STACK_t *g_sock_stack;
extern FD_SESSION_t *g_fd_session_table; 

static __thread INT32 uiThread_epfd = 0;

void shut_down(void){
    exit(1);
}
void work_clean(){
}
void work_thread(struct server_conf *srv){
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
		if(uiRet != OK || n <=0){
			//log_debug(LOG_LEVEL_DEBUG,"connection pop err,continue");
            sleep(1);
			continue;
		}
        for (i = 0; i < n; i++)
        {
            fd_session_init(newfd[i]);
            
        }
		
		if((nfds=epoll_wait(uiThread_epfd,events,MAX_CON,1000))>0){
			for(j=0;j<nfds;j++)
				process_events(events+j,srv);
		}

	}
	close(uiThread_epfd);
	//pthread_cleanup_pop(1);
}
//init_clientfd(fd)
int process_events(struct epoll_event* ev,struct server_conf *srv){
	char *request;
	http_request_line_t *request_line;
	http_response_t response;
	response.len=0;
	response.data=NULL;
	if(ev->events&EPOLLHUP){
		log_debug(LOG_LEVEL_DEBUG, "remove fd because EPOLLHUP , %d",ev->data.fd);
        fd_session_delete(ev->data.fd);
        close(ev->data.fd);
		return OK;
	}
	if(ev->events&EPOLLIN){
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
		if(recv(ev->data.fd,request,REQ_MAX,0)>0){
			log_debug(LOG_LEVEL_DEBUG,"receive  from fd %d",ev->data.fd);
			request_decode(ev->data.fd,request,request_line);
			http_response_create(srv,request_line,&response);
            
			goto end;
		}
		else{
			log_debug(LOG_LEVEL_DEBUG,"remove fd because read read error or peer close, %d",ev->data.fd);
			close(ev->data.fd);
			
			goto end;
		}
		end:
            fd_session_delete(ev->data.fd);
            close(ev->data.fd);// ±ÜÃâclose wait
			if(request){
				free(request);
				request=NULL;
			}
			if(request_line){
				free(request_line);
				request_line=NULL;
			}
			return 0;
	}
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
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLET;
    epoll_ctl(uiThread_epfd,EPOLL_CTL_ADD,fd,&ev);
    return OK;
}
int fd_session_delete(int fd)
{
    struct epoll_event ev;
    FD_SESSION_t *pstFd_session;
    pstFd_session = g_fd_session_table + fd;
    
    memset(pstFd_session,0,sizeof(FD_SESSION_t));
    log_debug(LOG_LEVEL_DEBUG,"fd_session_delete %u", fd);
    epoll_ctl(uiThread_epfd,EPOLL_CTL_DEL,fd,NULL);
    return OK;
}

