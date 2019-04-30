
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
OPTIMIZATION?=-O2

# Default settings
STD=-std=c99
WARN=-Wall -W
OPT=$(OPTIMIZATION)
#DEBUG=-g -ggdb
DEBUG=-g
DEPS=-I./deps/
LIBDEPS=-Ldeps -lae

FINAL_CFLAGS=$(STD) $(WARN) $(OPT) $(CFLAGS) $(DEBUG) $(DEPS)
FINAL_LDFLAGS=$(LDFLAGS) $(DEBUG) $(LIBDEPS)

ifeq ($(uname_S),Linux)
	FINAL_CFLAGS+= -lpthread -D_GNU_SOURCE
	FINAL_LDFLAGS+= -lpthread
endif

OCTOPUS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
OCTOPUS_AR=$(QUIET_AR)ar crv
OCTOPUS_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)
INSTALL=$(QUIET_INSTALL)$(INSTALL)

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_AR = @printf '    %b %b\n' $(LINKCOLOR)AR$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_INSTALL = @printf '    %b %b\n' $(LINKCOLOR)INSTALL$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
endif

AE_LIB=libae.a
OCTOPUS_LIB=liboctopus.a
OCTOPUS_OBJ=array.o buffer.o client.o common.o hash.o list.o logging.o networking.o octopus.o worker.o worker_pool.o object.o sds.o ioworker_pool.o ioworker.o

all: echo_server redis_server

$(OCTOPUS_LIB): $(OCTOPUS_OBJ)
	$(OCTOPUS_AR) $(OCTOPUS_LIB) $(OCTOPUS_OBJ) 1>&2

$(AE_LIB):
	cd deps/libae && make
	cp deps/libae/$(AE_LIB) deps

echo_server: ./echo_server/*.h ./echo_server/*.c $(OCTOPUS_LIB) $(AE_LIB) $(OCTOPUS_OBJ)
	cd echo_server && make

redis_server: ./redis_server/*.h ./redis_server/*.c $(OCTOPUS_LIB) $(AE_LIB) $(OCTOPUS_OBJ)
	cd redis_server && make

%.o: %.c
	$(OCTOPUS_CC) -c $<

clean:
	rm -rf $(OCTOPUS_LIB) *.o *.dSYM
	cd deps && rm -rf *.a
	cd deps/libae && make clean
	cd echo_server && make clean
	cd redis_server && make clean

.PHONY: clean

noopt:
	$(MAKE) OPTIMIZATION="-O0"

