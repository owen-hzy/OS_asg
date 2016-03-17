CC = gcc
CFLAGS = -Wall


scheduler : scheduler.o
	${CC} ${CFLAGS} $? -o $@

scheduler.o: scheduler.c
	${CC} ${CFLAGS} -c $?

clean:
	rm -f scheduler scheduler.o
