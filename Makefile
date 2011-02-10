CC=gcc
CFLAGS=-I"./include" -O3 -pedantic -Wall -c -fmessage-length=0 -std=gnu99 -fPIC -fvisibility=hidden -DNDEBUG
#CFLAGS=-I"./include" -g -pedantic -Wall -c -fmessage-length=0 -std=gnu99 -fPIC -fvisibility=hidden
REDIS_HOME=$(CURDIR)
PHP_EXT_BUILD=$(REDIS_HOME)/php/build

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
 LIBS=-lm -lrt
endif
ifeq ($(UNAME), Darwin)
 LIBS=-lm
endif

ifdef SINGLETHREADED
 CFLAGS += -DSINGLETHREADED
endif

libhansock: libhansock/batch.o libhansock/connection.o libhansock/ketama.o libhansock/md5.o libhansock/module.o libhansock/parser.o libhansock/buffer.o
	mkdir -p lib
	gcc -shared -o "lib/libhansock.so" ./libhansock/batch.o ./libhansock/buffer.o ./libhansock/connection.o ./libhansock/ketama.o ./libhansock/md5.o ./libhansock/module.o ./libhansock/parser.o $(LIBS)

c_test: libhansock test.o
	gcc -o test test.o -Llib -lhansock

c_client: libhansock
	gcc client.c -o client  `mysql_config --cflags --libs`

all: c_test c_client
	
clean:
	cd libhansock; rm -rf *.o
	rm -rf lib
	rm -rf test
	rm -rf test.o
	-find . -name *.pyc -exec rm -rf {} \;
	-find . -name *.so -exec rm -rf {} \;
	-find . -name '*~' -exec rm -rf {} \;
	
