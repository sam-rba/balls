#include <stdlib.h>
#include <stdio.h>

#include "balls.h"
#include "sysfatal.h"

/*
 * Read the file called filename. Sets contents to a malloc-allocated string containing
 * the contents of the file and a terminal '\0'. Sets size to strlen(contents) (excludes
 * '\0'). Returns non-zero on error.
 */
int
readFile(const char *filename, char **contents, size_t *size) {
	FILE *f;

	if ((f = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Failed to open file '%s'\n", filename);
		return 1;
	}
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	if ((*contents = malloc((*size + 1) * sizeof(char))) == NULL) {
		fclose(f);
		fprintf(stderr, "Failed to allocate file buffer for '%s'\n", filename);
		return 1;
	}
	rewind(f);
	fread(*contents, sizeof(char), *size, f);
	(*contents)[*size] = '\0';
	fclose(f);
	return 0;
}
