#ifndef WORK_H
#define WORK_H
#include "config.h"
#include <sys/epoll.h>
void work_thread();
int process_events(struct epoll_event *);
//void work_loop(struct server_conf *);
//void shutdown(struct server_conf *,struct connection);
void shut_down(void);
void work_clean(void);
int fd_session_init(int fd);
int fd_session_delete(int fd);

int fd_set_events(int fd);



#endif
