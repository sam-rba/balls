CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -Wno-deprecated-declarations
LDFLAGS = -lGLEW -lGL -lX11 -lGLU -lOpenGL -lOpenCL -lglut -lGLX

SRC = balls.c sysfatal.c geo.c rand.c partition.c
OBJ = ${SRC:.c=.o}

balls: ${OBJ}
	${CC} -o $@ ${LDFLAGS} $^

%.o: %.c
	${CC} -c ${CFLAGS} $<

clean:
	rm -f *.o balls

${OBJ}: sysfatal.h balls.h
