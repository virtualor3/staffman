source := main.c Server.c sql.c

all:
	gcc ${source} -o ser -pthread -lsqlite3

%.o:%.c
	gcc $^ -c $@