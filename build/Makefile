.PHONY: all clean
COMPILER = gcc
FLAGS = -D_XOPEN_SOURCE=700 -std=c11 -pedantic -Wall -Werror -pthread -g

all: server client

define log
	@echo $(1)
endef

clean: 
	$(call log,"cleaning bin...")
	rm -rf *.o server client

server: server.o handle-connection.o handle-request.o
		$(COMPILER) -o $@ $(FLAGS) $^

client: ../src/client.c
	$(call log,"building client...")
	$(COMPILER) -o $@ $(FLAGS) $<

server.o: ../src/server.c ../src/header.h
		$(call log,"building server...")
		$(COMPILER) -c $< $(FLAGS) 

handle-connection.o: ../src/handle-connection.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS) 

handle-request.o: ../src/handle-request.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS) 




