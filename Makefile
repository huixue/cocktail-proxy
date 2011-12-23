EXE = twproxy
all: $(EXE)

CC = g++
CFLAGS = -g -W -Wall -Wno-unused
LDFLAGS = -lpthread -lssl

OBJS = main.o MyServerSocket.o MySocket.o HTTPRequest.o http_parser.o HTTP.o Cache.o CacheEntry.o

-include $(OBJS:.o=.d)

twproxy: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS)

serv: serv.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) serv.o

%.d: %.c
	@set -e; $(CC) -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@;
	@[ -s $@ ] || rm -f $@

%.d: %.cpp
	@set -e; $(CC) -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@;
	@[ -s $@ ] || rm -f $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) *.o *~ core.* *.d
