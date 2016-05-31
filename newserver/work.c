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
extern CON_STACK_t g_connections;

static int set_nonblock(int fd){
    int flag;
    flag=fcntl(fd,F_GETFL);
    if(flag&O_NONBLOCK)
        return 0;
    else
        fcntl(fd,F_SETFL,O_NONBLOCK);
    return 0;
}
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
    
    struct epoll_event ev,*events;
    //pthread_cleanup_push(&work_clean,NULL);
    events=(struct epoll_event*)malloc(MAX_CON*sizeof(struct epoll_event));
    if(!events){
        log_debug(LOG_LEVEL_DEBUG,"events malloc failed");
        exit(-1);
    }
    epfd=epoll_create(256);
    if(-1==epfd){
        log_debug(LOG_LEVEL_DEBUG,"events malloc failed");
        exit(-1);
    }
	while(1){
	    n = 128;
        //pop array
		uiRet = con_pop_batch(&g_connections, newfd, &n);
		if(uiRet != OK || n <=0){
			//log_debug(LOG_LEVEL_DEBUG,"connection pop err,continue");
            sleep(1);
			continue;
		}
        for (i = 0; i < n; i++)
        {
            log_debug(LOG_LEVEL_DEBUG,"deal with %u", newfd[i]);
            set_nonblock(newfd[i]);
            ev.data.fd=newfd[i];
    		ev.events=EPOLLIN|EPOLLET;
    		epoll_ctl(epfd,EPOLL_CTL_ADD,newfd[i],&ev);
        }
		
		if((nfds=epoll_wait(epfd,events,MAX_CON,1000))>0){
			for(j=0;j<nfds;j++)
				process_events(epfd,events+j,srv);
		}

	}
	close(epfd);
	//pthread_cleanup_pop(1);
}
//init_clientfd(fd)
int process_events(int epfd,struct epoll_event* events,struct server_conf *srv){
	char *request;
	http_request_line_t *request_line;
	http_response_t response;
	response.len=0;
	response.data=NULL;
	if(events->events&EPOLLHUP){
		log_debug(LOG_LEVEL_DEBUG, "remove fd because EPOLLHUP , %d",events->data.fd);
		close(events->data.fd);
		
		//goto end;
	}
	if(events->events&EPOLLIN){
        epoll_ctl(epfd,EPOLL_CTL_DEL,events->data.fd,NULL);
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
		if(recv(events->data.fd,request,REQ_MAX,0)>0){
			log_debug(LOG_LEVEL_DEBUG,"receive  from fd %d",events->data.fd);
			request_decode(events->data.fd,request,request_line);
			http_response_create(srv,request_line,&response);
            
			goto end;
		}
		else{
			log_debug(LOG_LEVEL_DEBUG,"remove fd because read read error or peer close, %d",events->data.fd);
			close(events->data.fd);
			
			goto end;
		}
		end:
            epoll_ctl(epfd,EPOLL_CTL_DEL,events->data.fd,NULL);
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
