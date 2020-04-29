first: allmake
all: client server

client: *.c *.h
	gcc -g -Wall -Werror -std=gnu99 -ftrapv -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -o client client.c

server: *.c *.h
	gcc -g -Wall -Werror -std=gnu99 -ftrapv -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -o server server.c
