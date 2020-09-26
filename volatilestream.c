/*
 *   volatilestreaam:
 *   volatile stream = stdio FILE* stream as a temporary dynamically allocated
 *   (and deallocated) memory buffer
 *
 *   Copyright (C) 20182020  Renzo Davoli <renzo@cs.unibo.it> VirtualSquare team.
 *
 *   This library is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation; either version 2.1 of the License, or (at
 *   your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <volatilestream.h>

#define VOLSTREAM_PAGESIZE 4096

struct volstream {
	char *buf;
	size_t bufsize;
	size_t filelen;
	size_t filepos;
};

static ssize_t volstream_read(void *cookie, char *buf, size_t size) {
	struct volstream *vols = cookie;
	ssize_t ret_value;
	if (vols->filepos + size <= vols->filelen)
		ret_value = size;
	else
		ret_value = vols->filelen - vols->filepos;
	if (ret_value > 0) {
		memcpy(buf, vols->buf + vols->filepos, ret_value);
		vols->filepos += ret_value;
	}
	return ret_value;
}

static inline void volstream_buf_expand(struct volstream *vols, size_t newfilesize) {
	if (newfilesize > vols->bufsize) {
		size_t newsize = (newfilesize + (VOLSTREAM_PAGESIZE - 1)) & ~(VOLSTREAM_PAGESIZE - 1);
    char *newbuf;
    newbuf = realloc(vols->buf, newsize);
    if (newbuf != NULL) {
      vols->buf = newbuf;
      vols->bufsize = newsize;
		}
	}
}

static inline void volstream_buf_shrink(struct volstream *vols, size_t newfilesize) {
	size_t newsize = (newfilesize + (VOLSTREAM_PAGESIZE - 1)) & ~(VOLSTREAM_PAGESIZE - 1);
	if (newsize == 0)
		newsize = VOLSTREAM_PAGESIZE;
	if (newsize < vols->bufsize - VOLSTREAM_PAGESIZE) {
		char *newbuf;
    newbuf = realloc(vols->buf, newsize);
    if (newbuf != NULL) {
      vols->buf = newbuf;
      vols->bufsize = newsize;
    }
	}
}

static ssize_t volstream_write(void *cookie, const char *buf, size_t size) {
	struct volstream *vols = cookie;
	ssize_t ret_value;
	volstream_buf_expand(vols, vols->filepos + size);
	if (vols->filepos + size <= vols->bufsize)
		ret_value = size;
	else
		ret_value = vols->bufsize - vols->filepos;
	if (ret_value > 0) {
		memcpy(vols->buf + vols->filepos, buf, ret_value);
		vols->filepos += ret_value;
		if (vols->filepos > vols->filelen)
			vols->filelen = vols->filepos;
	}
	return ret_value;
}

static int volstream_seek(void *cookie, off64_t *offset, int whence) {
	struct volstream *vols = cookie;
	off64_t newpos;
	switch (whence) {
		case SEEK_SET: newpos = *offset; break;
		case SEEK_CUR: newpos = vols->filepos + *offset; break;
		case SEEK_END: newpos = vols->filelen + *offset; break;
		default:       return errno = EINVAL, -1;
	}
	if (newpos < 0) {
		return errno = EINVAL, -1;
	} else {
		if (newpos > vols->filelen) {
			volstream_buf_expand(vols, newpos);
			if (newpos > vols->bufsize)
				return errno = EINVAL, -1;
			if (newpos > vols->filelen) {
				memset(vols->buf + vols->filelen, 0, newpos - vols->filelen);
				vols->filelen = newpos;
			}
		}
		vols->filepos = newpos;
		*offset = newpos;
		return 0;
	}
}

static int volstream_close(void *cookie) {
	struct volstream *vols = cookie;
	free(vols->buf);
	free(vols);
	return 0;
}

int volstream_trunc(struct volstream *vols, size_t length) {
	if (length < 0)
    return errno = EINVAL, -1;
  else {
		if (length > vols->filelen) {
			volstream_buf_expand(vols, length);
			if (length > vols->bufsize)
				return errno = ENOMEM, -1;
			if (length > vols->filelen)
				memset(vols->buf + vols->filelen, 0, length - vols->filelen);
		}
		vols->filelen = length;
		volstream_buf_shrink(vols, length);
		return 0;
	}
}

void *volstream_getbuf(struct volstream *vols) {
	return vols->buf;
}

size_t volstream_getsize(struct volstream *vols) {
	return vols->filelen;
}

static cookie_io_functions_t volstream_f = {
	.read = volstream_read,
	.write = volstream_write,
	.seek = volstream_seek,
	.close = volstream_close
};

FILE *volstream_openv(struct volstream **_vols) {
	FILE *volstream;
	struct volstream *vols = malloc(sizeof(struct volstream));
	if (vols == NULL)
		goto err;
	vols->buf = malloc(VOLSTREAM_PAGESIZE);
	if (vols->buf == NULL)
		goto buferr;
	vols->bufsize = VOLSTREAM_PAGESIZE;
	vols->filelen = 0;
	vols->filepos = 0;
	volstream = fopencookie(vols, "r+", volstream_f);
	if (volstream == NULL)
		goto volstreamerr;
	if (_vols != NULL)
		*_vols = vols;
	return volstream;
volstreamerr:
	free(vols->buf);
	free(vols);
	return NULL;
buferr:
	free(vols);
err:
	errno = ENOMEM;
	return NULL;
}

FILE *volstream_open(void) {
	return volstream_openv(NULL);
}

#if 0
int main(int argc, char *argv[]) {
	FILE *f = volstream_open();
	int c;
	for (argv++; *argv; argv++) {
		fprintf(f, "%s\n", *argv);
		fflush(f);
	}
	fseek(f, 0, SEEK_SET);
	while ((c = getc(f)) != EOF)
		putchar(c == 0 ? '.' : c);
	fclose(f);
}
#endif
#if 0
int main(int argc, char *argv[]) {
	struct volstream *vols;
	FILE *f = volstream_openv(&vols);
	int c;
	for (argv++; *argv; argv++) {
		fprintf(f, "%s\n", *argv);
		fflush(f);
	}
	fprintf(f,"FINE");
	fflush(f);
	volstream_trunc(vols, 100);
	ssize_t s = volstream_getsize(vols);
	char *buf = volstream_getbuf(vols);
	ssize_t i;
	for (i=0; i<s; i++)
		putchar(buf[i] == 0 ? '.' : buf[i]);
	printf("\n");
	fseek(f, 0, SEEK_SET);
	while ((c = getc(f)) != EOF)
		putchar(c == 0 ? '.' : c);
	fclose(f);
	printf("\n");
}
#endif
