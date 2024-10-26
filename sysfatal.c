#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "sysfatal.h"

void
sysfatal(const char * format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fflush(NULL);
	exit(1);
}
