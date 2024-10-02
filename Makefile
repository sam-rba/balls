CC = g++
CFLAGS = -Wall -pedantic
LDFLAGS = -ltbb -lglut -lGLU -lGL

balls: balls.o
	${CC} -o $@ $^ ${LDFLAGS}
	@echo done

%.o: %.cpp
	${CC} -c ${CFLAGS} $<