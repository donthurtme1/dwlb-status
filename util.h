#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

void
die(const char *const msg) {
	register const char *c = msg;
	register int i = 0;

	while (*c != '\0' && i < 256) {
		i++;
		c++;
	}

	cleanup();
	if (*(c - 1) != '\n') {
		perror(msg);
	} else {
		printf("%s", msg);
	}
	exit(EXIT_FAILURE);
}

#endif
