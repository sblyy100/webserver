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
    warn_log("exit...");
    exit(1);
}
void work_clean(){
    warn_log("work thread exit...");
}
void work_thread(struct server_conf *srv){
    char logstr[1024];
    int epfd;//epoll fd
    int newfd;//pop the accept fd
    int nfds;//epoll wait return fd
    int i;
    extern struct con_stack *connections;
    struct epoll_event ev,*events;
    //pthread_cleanup_push(&work_clean,NULL);
    events=(struct epoll_event*)malloc(MAX_CON*sizeof(struct epoll_event));
    if(!events){
        error_log("events malloc failed");
        exit(-1);
    }
    epfd=epoll_create(256);
    if(-1==epfd){
        error_log("events malloc failed");
        exit(-1);
    }
	while(1){
	#if 1
        if(NULL==connections){
            log_debug(LOG_LEVEL_DEBUG,"connctions not create!");
            sleep(1);
            continue;
        }
	#endif
        if(connections->top<0){
            sleep(1);
            continue;
        }
		newfd=con_pop(connections);
		if(newfd<0){
			sprintf(logstr,"newfd error,%d",newfd);
			warn_log(logstr);
			continue;
		}
		set_nonblock(newfd);
		ev.data.fd=newfd;
		ev.events=EPOLLIN|EPOLLET;
		epoll_ctl(epfd,EPOLL_CTL_ADD,newfd,&ev);
		if((nfds=epoll_wait(epfd,events,MAX_CON,1000))>0){
			for(i=0;i<nfds;i++)
				process_events(epfd,events+i,srv);
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
		epoll_ctl(epfd,EPOLL_CTL_DEL,events->data.fd,NULL);
		//goto end;
	}
	if(events->events&EPOLLIN){
		request=(char *)calloc(REQ_MAX,sizeof(char));
		if(!request){
			log_debug(LOG_LEVEL_DEBUG,"request malloc failed");
			goto end;
		}
		log_debug(LOG_LEVEL_DEBUG,"malloc for request buffer ok!");
		request_line=(http_request_line_t*)calloc(1,sizeof(http_request_line_t));
		if(!request_line){
			error_log("malloc for request decode buf error");
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
			epoll_ctl(epfd,EPOLL_CTL_DEL,events->data.fd,NULL);
			goto end;
		}
		end:
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
