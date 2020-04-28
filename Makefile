first: allmake
all: program client server

program: main.c
	gcc -g -Wall -Werror -std=gnu99 -ftrapv -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -o minifs main.c

client: client.c
	gcc -g -Wall -Werror -std=gnu99 -ftrapv -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -o client client.c

server: server.c
	gcc -g -Wall -Werror -std=gnu99 -ftrapv -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -o server server.c
