/*
 *   volatilestreaam: 
 *   volatile stream = stdio FILE* stream as a temporary dynamically allocated 
 *   (and deallocated) memory buffer
 *
 *   Copyright (C) 2018  Renzo Davoli <renzo@cs.unibo.it> VirtualSquare team.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>

struct volstream {
	FILE *f;
	char *buf;
	size_t size;
};

static ssize_t volstream_read(void *cookie, char *buf, size_t size) {
	struct volstream *vols = cookie;
	clearerr(vols->f);
	ssize_t ret_value = fread(buf, 1, size, vols->f);
	if (ret_value == 0 && ferror(vols->f) != 0)
		ret_value = -1;
	return ret_value;
}

static ssize_t volstream_write(void *cookie, const char *buf, size_t size) {
	struct volstream *vols = cookie;
	return fwrite(buf, 1, size, vols->f);
}

static int volstream_seek(void *cookie, off64_t *offset, int whence) {
	struct volstream *vols = cookie;
	int ret_value = fseeko(vols->f, *offset, whence);
	if (ret_value == 0)
		*offset = ftello(vols->f);
	return ret_value;
}

static int volstream_close(void *cookie) {
	struct volstream *vols = cookie;
	int ret_value = fclose(vols->f);
	if (ret_value == 0) {
		if (vols->buf)
			free(vols->buf);
		free(vols);
	}
	return ret_value;
}

static cookie_io_functions_t volstream_f = {
	.read = volstream_read,
	.write = volstream_write,
	.seek = volstream_seek,
	.close = volstream_close
};

FILE *volstream_open(void) {
	FILE *volstream;
	struct volstream *vols = malloc(sizeof(struct volstream));
	if (vols == NULL)
		goto err;
	vols->buf = NULL;
	vols->size = 0;
	vols->f = open_memstream(&vols->buf, &vols->size);
	if (vols->f == NULL)
		goto errmemstream;
	volstream = fopencookie(vols, "r+", volstream_f);
	if (volstream == NULL)
		goto errvolstream;
	return volstream;
errvolstream:
	fclose(vols->f);
errmemstream:
	free(vols);
err:
	return NULL;
}

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

