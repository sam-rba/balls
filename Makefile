CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -Wno-deprecated-declarations
LDFLAGS = -lOpenCL -lglut -lGLU -lGL -lGLX

SRC = balls.c sysfatal.c
OBJ = ${SRC:.c=.o}

balls: ${OBJ}
	${CC} -o $@ ${LDFLAGS} $^

%.o: %.c
	${CC} -c ${CFLAGS} $<

clean:
	rm -f *.o balls

${OBJ}: sysfatal.h
