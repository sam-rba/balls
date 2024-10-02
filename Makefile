CC = g++
CFLAGS = -Wall -pedantic
LDFLAGS = -ltbb -lglut -lGLU -lGL

balls: balls.o collision.o
	${CC} -o $@ $^ ${LDFLAGS}
	@echo done

%.o: %.cpp balls.h
	${CC} -c ${CFLAGS} $<