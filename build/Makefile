.PHONY: all clean
COMPILER = clang
FLAGS = -D_XOPEN_SOURCE=700 -std=c11 -pedantic -Wall -Werror -pthread -g

all: server client

define log
	@echo $(1)
endef

clean: 
	$(call log,"cleaning bin...")
	rm -rf *.o server client
	$(call log,"bin cleaned successfully")


server: server.o handle-request.o logger.o
		$(call log,"building server...")
		$(COMPILER) -o $@ $(FLAGS) $^
		$(call log,"server built successfully")	

client: client.o logger.o
	$(call log,"building client...")
	$(COMPILER) -o $@ $(FLAGS) $^
	$(call log,"client built successfully")

client.o:../src/client.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS)


server.o: ../src/server.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS) 
		

handle-connection.o: ../src/handle-connection.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS) 

handle-request.o: ../src/handle-request.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS) 

logger.o: ../src/logger.c ../src/header.h
		$(COMPILER) -c $< $(FLAGS) -DLOG_USE_COLOR



