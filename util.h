#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
die(const char *const msg) {
	char *whole_message = (char *)malloc(strlen(msg) + sizeof "ERROR: ");
	strcpy(whole_message, "ERROR: ");
	strcat(whole_message, msg);

	perror(whole_message);
	fflush(stderr);
	exit(1);
}

#endif
