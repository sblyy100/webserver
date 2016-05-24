AUTOMAKE_OPTIONS=foreign
#SUBDIRS=src
bin_PROGRAMS=myhttpd
myhttpd_SOURCES=config.c  listen.c  log.c  response.c  server.c  unix_sock.c  watch.c  work.c http.c network.c
INCLUDES=-I/usr/local/include/libxml2
myhttpd_LDADD=-L/usr/local/lib -lxml2 -lz -lm -lpthread
