#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "log.h"
#include "unix_sock.h"
UINT32 init_conf(struct server_conf *server){
	//init_conf(&server)
	char *tmp;
	if(!server){
        log_debug(LOG_LEVEL_DEBUG,"server conf is null");
		return ERR;
	}
	server->ip=read_conf("ip");
	tmp=read_conf("port");
	server->port=atoi(tmp);
	free(tmp);
	server->docroot=read_conf("docroot");
	server->logdir=read_conf("logdir");
	tmp=read_conf("maxfds");
	server->maxfds=atoi(tmp);
	free(tmp);
    return OK;
	
}
void destory_conf(struct server_conf *server){
	free(server->ip);
	server->ip=NULL;
	free(server->docroot);
	server->docroot=NULL;
	free(server->logdir);
	server->logdir=NULL;
	free(server);
	server=NULL;
}
char *read_conf(char *key){
	xmlDocPtr doc;
	xmlNodePtr root,cur;
	xmlChar *content;
	if(!(doc=xmlParseFile(configfile))){
	log_debug(LOG_LEVEL_DEBUG, "open config error");
	exit(-1);
	}
	if(xmlKeepBlanksDefault(0)){
		log_debug(LOG_LEVEL_DEBUG, "remove xml blank failed\n");
	}
	root=xmlDocGetRootElement(doc);
	if(!root)
		log_debug(LOG_LEVEL_DEBUG,"server xml is empty!");
	if(xmlStrcmp(root->name,BAD_CAST"server")!=0){
		log_debug(LOG_LEVEL_DEBUG,"xml parser failed,root element is not server");
		exit(-1);
	}
	cur=root->xmlChildrenNode;
	while(cur){
		if(xmlStrcmp(cur->name,BAD_CAST(key))==0){
			if(!(content=(xmlChar *)malloc(128*sizeof(char))))//free when exit
				log_debug(LOG_LEVEL_DEBUG,"xml content malloc error!");
				
			strcpy(content,xmlNodeGetContent(cur));
			xmlFreeDoc(doc);
			return (char *)content;
		}
		cur=cur->next;
	}
	xmlFreeDoc(doc);
	return NULL;
}
static void do_cmd(char *cmd){
	if(!strcmp(cmd,"stop")){
	    log_debug(LOG_LEVEL_DEBUG, "receive a stop cmd!");
		;
	}
	else if(!strcmp(cmd,"restart")){
		log_debug(LOG_LEVEL_DEBUG,"receive a restart cmd!");
	    ;
	}
	else
	    log_debug(LOG_LEVEL_DEBUG, "receive unknow cmd!");
}

void recv_cmd_loop(){
	recv_process(config_with_ctl,do_cmd,NULL);//NULL is do_cmd
}
#ifdef CONFIG_DEBUG
#include <assert.h>
main(){
	struct server_conf *server;
	server=calloc(1,sizeof(struct server_conf));
	assert(server);
	init_conf(server);

	printf("config:%s=%s\n","ip",server->ip);
	printf("config:%s=%d\n","port",server->port);
	printf("config:%s=%s\n","docroot",server->docroot);
	printf("config:%s=%s\n","logdir",server->logdir);
	printf("config:%s=%d\n","maxfds",server->maxfds);

	
	destory_conf(server);
}
#endif
