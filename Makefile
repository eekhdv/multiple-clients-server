CC=gcc

WARNING=-Wall -Wextra -Werror
FLAGS=$(WARNING) -pthread -O2
SRC_BACKGROUND=src/server.c src/utils.c src/init.c
SRC=src/app.c $(SRC_BACKGROUND)

DEPS=$(SRC) src/server.h src/utils.h src/init.h
BINARY=./app

$(BINARY): $(DEPS)
	$(CC) $(FLAGS) $(DEPS) -o $@
