#include <sys/socket.h>
#include <arpa/inet.h>
#include "listen.h"
#include "log.h"
#include "network.h"
#include <stdlib.h>
//int listen_sock=0;
struct con_stack *connections;
int listen_thread(struct server_conf *srv){
	struct sockaddr_in server;
	struct sockaddr_in client;
	//struct con_stack *connections=NULL;
	int client_len;
	int newsock=-1;
	int sock;
	char logstr[1024];
	server.sin_family=AF_INET;    ///init server addr
	//struct con_stack *connections;
	log_debug(LOG_LEVEL_DEBUG, "listen thread create %u",pthread_self());
	connections=(struct con_stack*)malloc(sizeof(struct con_stack));
	if(!connections){
		log_debug(LOG_LEVEL_DEBUG,"malloc for connection stack error");
		exit(-1);
	}
	log_debug(LOG_LEVEL_DEBUG, "malloc for connection stack OK");
	stack_init(connections);
	if(!srv->ip)
    		server.sin_addr.s_addr=htonl(INADDR_ANY);
	else
		inet_pton(AF_INET,srv->ip,&server.sin_addr.s_addr);
   	if(!srv->port)
		server.sin_port=htons(80);
	else
		server.sin_port=htons(srv->port);
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
	if(!srv->maxfds){
		listen(sock,10);
		
	}
	else{
		listen(sock,srv->maxfds);
		log_debug(LOG_LEVEL_DEBUG, "listen sucessful");
	}
	while(1){
		if((int)(connections->top)>128){
			//printf("top is %d\n",connections->top);
			log_debug(LOG_LEVEL_DEBUG, "listen stack > 128,sleep 1s");
			sleep(1);
			continue;
		}
		if((newsock=accept(sock,(struct sockaddr*)&client,&client_len))<0){
			log_debug(LOG_LEVEL_DEBUG, "accept error,accept sock %d",newsock);
			continue;
		}
		log_debug(LOG_LEVEL_DEBUG,"accept OK,accept sock %d",newsock);
		
		con_push(connections,newsock);

	}
}
