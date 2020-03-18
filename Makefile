first: allmake
all: build

build: main.c
	gcc -g -Wall -Werror -std=gnu99 -ftrapv -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -o minifs main.c
