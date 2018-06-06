#ifndef VOLATILESTREAM_H
#define VOLATILESTREAM_H

/* VOLATILE STREAM

	 The volstream_open function opens a stdio stream as a temporary memory buffer.
	 The buffer is dynamically allocated, grows as needed and it is automatically
	 deallocated when the stream is closed.

	 */

/* the following example writes all the command arguments in a volatile stream,
	 then it rereads the volatile stream one byte at a time */
#if 0
int main(int argc, char *argv[]) {
	FILE *f = volstream_open();
	int c;
	for (argv++; *argv; argv++) 
		fprintf(f, "%s\n", *argv);
	fseek(f, 0, SEEK_SET);
	while ((c = getc(f)) != EOF)
		putchar(c);
	fclose(f);
}
#endif

#include <stdio.h>

FILE *volstream_open(void);

#endif
