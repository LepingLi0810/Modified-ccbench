SRC = src
INCLUDE = include

CFLAGS = -O3 -Wall
LDFLAGS = -lm -lrt -lpthread 
VER_FLAGS = -D_GNU_SOURCE

ifeq ($(VERSION),DEBUG) 
CFLAGS =  -O0 -ggdb -Wall -g  -fno-inline
endif

UNAME := $(shell uname -n)

ifeq ($(UNAME), lpd48core)
PLATFORM = OPTERON
CC = gcc
PLATFORM_NUMA = 1
endif

ifeq ($(UNAME), diassrv8)
PLATFORM = XEON
CC = gcc
PLATFORM_NUMA = 1
endif

ifeq ($(UNAME), maglite)
PLATFORM = NIAGARA
CC = /opt/csw/bin/gcc 
CFLAGS += -m64 -mcpu=v9 -mtune=v9
endif

ifeq ($(UNAME), parsasrv1.epfl.ch)
PLATFORM = TILERA
CC = tile-gcc
LDFLAGS += -ltmc
endif

ifeq ($(UNAME), diascld19)
PLATFORM = XEON2
CC = gcc
endif

ifeq ($(UNAME), diascld9)
PLATFORM = OPTERON2
CC = gcc
endif

ifeq ($(PLATFORM), )
PLATFORM = DEFAULT
CC = gcc
endif

VER_FLAGS += -D$(PLATFORM)

ifeq ($(PLATFORM_NUMA),1) #give PLATFORM_NUMA=1 for NUMA
LDFLAGS += -lnuma
VER_FLAGS += -DPLATFORM_NUMA
endif 

default: ccbench

all: ccbench

ccbench: ccbench.o $(SRC)/pfd.c $(SRC)/barrier.c $(INCLUDE)/common.h $(INCLUDE)/ccbench.h $(INCLUDE)/pfd.h $(INCLUDE)/barrier.h barrier.o pfd.o
	$(CC) $(VER_FLAGS) -o ccbench ccbench.o pfd.o barrier.o ./liblfds711/obj/*.o $(CFLAGS) $(LDFLAGS) -I./$(INCLUDE) -I./liblfds711/inc 
ccbench.o: $(SRC)/ccbench.c $(INCLUDE)/ccbench.h
	$(CC) $(VER_FLAGS) -c $(SRC)/ccbench.c $(CFLAGS) -I./$(INCLUDE) -I./liblfds711/inc 

pfd.o: $(SRC)/pfd.c $(INCLUDE)/pfd.h
	$(CC) $(VER_FLAGS) -c $(SRC)/pfd.c $(CFLAGS) -I./$(INCLUDE) -I./liblfds711/inc 

barrier.o: $(SRC)/barrier.c $(INCLUDE)/barrier.h
	$(CC) $(VER_FLAGS) -c $(SRC)/barrier.c $(CFLAGS) -I./$(INCLUDE) -I./liblfds711/inc 
main:
	gcc -Iliblfds711/inc main.c ./liblfds711/obj/*.o -lpthread -fsanitize=address -o main

clean:
	rm -f *.o ccbench main
