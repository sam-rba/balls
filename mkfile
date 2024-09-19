CC=9c
LD=9l
O=o

OBJ = balls.$O vec.$O collision.$O

balls: $OBJ
	$LD -o balls $OBJ

%.$O: %.c
	$CC $CFLAGS $stem.c

clean:V:
	rm -f ./*.$O balls