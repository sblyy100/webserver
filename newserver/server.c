#include "log.h"
#include "server.h"
#include "unix_sock.h"
#include "listen.h"
#include "watch.h"
#include "work.h"
#include "network.h"
#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>
#define WORK_PROCESS_NUM 1
#define WORK_THREAD_NUM 2
#define MODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
extern int errno;
extern int log_to;
extern log_t *dlog;

pthread_t gui_listen_tid = 0;
pthread_t gui_config_tid = 0;
pthread_t gui_worker_tid[WORK_THREAD_NUM] = {0};

int isdaemon = 1;
extern CON_STACK_t g_connections;

static void sigint_handler(int sig){
    log_debug(LOG_LEVEL_DEBUG,"shutdown");
	fini_log();
	shut_down();
	//warn_log("shut down!!");
}
static void mydaemon(void) {
	int fd;
	if (0 != fork()) exit(0);//fork使子进程继承父进程的进程组，但是新的pid
	if (-1 == setsid()) exit(0);//设置回话id，使其成为新会话的首进程，进程组组长，失去控制终端
	signal(SIGHUP, SIG_IGN);
	if (0 != fork()) exit(0);//有 则pgid=sid,但是pid不等。  没有这句话，则pid=pgid=sid
	//为了防止进程再次获得控制终端，也可以只fork一次
	//if (0 != chdir("/")) exit(0);
	
	fd=open("/dev/null",O_RDWR);
	dup2(fd,STDIN_FILENO);
	dup2(fd,STDOUT_FILENO);
}
static void usage(void){
	printf("usage:\n \
	-c configfile : set the config file path,default is current dir\n \
	-p : print config\n \
	-l logpath : set the log path,default is /var/log/server.log\n \
	-d : debug mode\n");
}
static void parse_arg(int argc,char **argv){
	char ch;
	if(argc<2){
		usage();
		exit(-1);
	}
	while((ch=getopt(argc,argv,"c:pl:d"))!=-1){
		switch(ch){
			case 'c':
				printf("config file is : %s\n",optarg);
				break;
			case 'p':
				printf("print config\n");
				break;
			case 'l':
				if(!strcmp(optarg,"stderr"))
					log_to=LOG_2_STDERR;
				printf("log path is : %s\n",optarg);
				break;
			case 'd':
				printf("run in debug mode\n");
				isdaemon=0;
				//log_to=LOG_2_STDERR;
				break;
			default:
				printf("no arg\n");
				break;
		}
	}
}
main(int argc,char **argv){
	UINT32 tmp;
    UINT32 uiRet;
	char tmp_buf[128];
	INT32 child=0;
	UINT32 child_num=WORK_PROCESS_NUM;
	pid_t pid;				//the pid of worker child process
	
	/*start main thread*/
	//extern struct server_conf  srv;
	/*sig init*/
	signal(SIGINT,sigint_handler);
    signal(SIGTERM,sigint_handler);
    signal(SIGHUP,sigint_handler);
	parse_arg(argc,argv);
	if(isdaemon)
		mydaemon();
    
	uiRet = init_log();
    if (uiRet != OK)
    {
        perror("init  log err");
        exit(-1);
    }
	uiRet = init_conf(&srv);
    if (uiRet != OK)
    {
        log_debug(LOG_LEVEL_DEBUG,"init server config err");
        exit(-1);
    }
	
	
	#if 0
	if(pthread_create(&config_tid,NULL,recv_cmd_loop,NULL)<0){
		log_debug(LOG_LEVEL_DEBUG,"create config thread error");
		//exit(-1);
	}
    
	log_debug(LOG_LEVEL_DEBUG,"create config thread sucessful");
    #endif
	/*main thread*/
	/*parent process:watch*/
	while(isdaemon&&(!child)){
		if(child_num>0){
			switch((pid=fork())){
				case -1:
					return -1;
				case 0:
					child=1;
					break;
				default:
					child_num--;
					tmp=open("/var/run/server.pid",O_RDWR|O_CREAT,MODE);
					sprintf(tmp_buf,"%d",pid);
					write(tmp,tmp_buf,strlen(tmp_buf));
					close(tmp);
					break;
			}
		}
		else{
			wait(NULL);
			child_num++;
		}
	}
	if((!isdaemon)||(child&&isdaemon))
    {   
        stack_init(&g_connections);
        
        pthread_create(&gui_listen_tid,NULL,listen_thread,&srv);
        pthread_create(&gui_worker_tid[0],NULL,work_thread,&srv);
        pthread_create(&gui_worker_tid[1],NULL,work_thread,&srv);
        //wait listen and worker
        pthread_join(gui_listen_tid,NULL);
        pthread_join(gui_worker_tid[0],NULL);
        pthread_join(gui_worker_tid[1],NULL);
        //work_loop(&srv);
	}
}
