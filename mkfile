CC=9c
LD=9l
O=o

balls: balls.$O
	$LD -o balls balls.$O

%.$O: %.c
	$CC $CFLAGS $stem.c

clean:V:
	rm -f balls.$O balls