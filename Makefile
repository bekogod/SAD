CC=gcc
CFLAGS= -Wall
DEPS = metaindex.h remote.h
OBJP = passthrough.o metaindex.o remote.o
OBJS = storserver.o remote.o

LIBS=`pkg-config --cflags --libs glib-2.0` `pkg-config fuse3 --cflags --libs` -lpthread -lcrypto

all: passthrough storserver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

passthrough: $(OBJP)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

storserver: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -rf *.o passthrough storserver /backend/* /home/vagrant/server/* /home/vagrant/server2/*