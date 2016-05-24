CFLAGS+=-L/usr/lib -lxml2  -lm -lpthread -I/usr/include/libxml2

objects =config.o  decode.o  http.o  listen.o  log.o  network.o  response.o  server.o  unix_sock.o  watch.o  work.o

server: $(objects)
	$(CC) -o server  $(objects) $(CFLAGS)
$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@ $(CFLAGS)
clean:
	rm *.o
