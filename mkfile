CC=9c
LD=9l
O=o
LDFLAGS=

OBJ = balls.$O vec.$O collision.$O graphics.$O

balls: $OBJ
	$LD -o balls $OBJ $LDFLAGS

%.$O: %.c
	$CC $CFLAGS $stem.c

OBJ: balls.h

clean:V:
	rm -f ./*.$O balls