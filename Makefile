CC=gcc
CLANG_FORMAT=clang-format

WARNING=-Wall -Wextra -Werror
FLAGS=$(WARNING) -pthread -O2
SRC_BACKGROUND=src/server.c src/utils.c src/init.c
SRC=src/app.c $(SRC_BACKGROUND)
CODE_STYLE=-style="WebKit"

BINARY=./app

$(BINARY): $(SRC)
	$(CC) $(FLAGS) $(SRC) -o $@

.PHONY: format
format: $(DEPS)
	$(CLANG_FORMAT) -i $(CODE_STYLE) $(SRC)
