CC=cc
RPC_SYSTEM=rpc.o
LDFLAGS=-L.
LINKERS = client.a server.a 

.PHONY: format all

all: $(RPC_SYSTEM)

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) $(LDFLAGS) -Wall -c -o $@ $< 

rpc-client: rpc.o
	$(CC) $(CFLAGS) -o rpc-client rpc.o -L. -l:client.a

rpc-server: rpc.o
	$(CC) $(CFLAGS) -o rpc-server rpc.o -L. -l:server.a
# RPC_SYSTEM_A=rpc.a
# $(RPC_SYSTEM_A): rpc.o
# 	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

format:
	clang-format -style=file -i *.c *.h

clean: 
	rm -f *.o