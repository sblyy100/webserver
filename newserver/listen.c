#include <sys/socket.h>
#include <arpa/inet.h>
#include "listen.h"
#include "log.h"
#include "network.h"
#include <stdlib.h>
//int listen_sock=0;
extern CON_STACK_t *g_sock_stack;
extern SERVER_CONF_T srv;

int listen_thread(){
	struct sockaddr_in server;
	struct sockaddr_in client;
    SERVER_CONF_T *s = &srv;
	//struct con_stack *connections=NULL;
	int client_len;
	int newsock=-1;
	int sock;
	server.sin_family=AF_INET;    ///init server addr

	log_debug(LOG_LEVEL_DEBUG, "listen thread create %u",pthread_self());
	
	if(!s->ip)
    		server.sin_addr.s_addr=htonl(INADDR_ANY);
	else
		inet_pton(AF_INET,s->ip,&server.sin_addr.s_addr);
   	if(!s->port)
		server.sin_port=htons(80);
	else
		server.sin_port=htons(s->port);
   	memset(server.sin_zero,0,8);

	if((sock=socket(AF_INET,SOCK_STREAM,0))<0){
		log_debug(LOG_LEVEL_DEBUG,"create sock failed");
		exit(-1);
	}
	log_debug(LOG_LEVEL_DEBUG, "%s", "create sock sucessful");
	//listen_sock=sock;
	if(bind(sock,(struct sockaddr*)&server,sizeof(server))<0){
		log_debug(LOG_LEVEL_DEBUG,"bind sock error");
		exit(-1);
	}
	log_debug(LOG_LEVEL_DEBUG,"bind sock sucessful");
	if(!s->maxfds){
		listen(sock,10);
		
	}
	else{
		listen(sock,s->maxfds);
		log_debug(LOG_LEVEL_DEBUG, "listen sucessful,backlog:%u", s->maxfds);
	}
	while(1){
		if(g_sock_stack->top>128){
			//printf("top is %d\n",connections->top);
			log_debug(LOG_LEVEL_DEBUG, "listen stack %u,sleep 1s", g_sock_stack->top);
			sleep(1);
			continue;
		}
		if((newsock=accept(sock,(struct sockaddr*)&client,&client_len))<0){
			log_debug(LOG_LEVEL_DEBUG, "accept error,accept sock %d",newsock);
			continue;
		}
		log_debug(LOG_LEVEL_DEBUG,"accept OK,accept sock %d",newsock);
		
		con_push(g_sock_stack,newsock);

	}
}
