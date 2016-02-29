CC = gcc
CFLAGS = -Wall


shell : shell.o
	${CC} ${CFLAGS} $? -o $@

shell.o: shell.c
	${CC} ${CFLAGS} -c $? 

clean:
	rm -f shell shell.o
