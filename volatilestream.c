/*
 *   volatilestreaam:
 *   volatile stream = stdio FILE* stream as a temporary dynamically allocated
 *   (and deallocated) memory buffer
 *
 *   Copyright (C) 2018-2023  Renzo Davoli <renzo@cs.unibo.it> VirtualSquare team.
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
#include <pthread.h>
#include <volatilestream.h>

#define VOLSTREAM_MINBUFSIZE 256

struct volstream {
	FILE *f;
	char *buf;
	size_t bufsize;
	size_t filelen;
	size_t filepos;
	struct volstream *next, **pprev;
	pthread_mutex_t mutex;
};

static pthread_rwlock_t vols_lock = PTHREAD_RWLOCK_INITIALIZER;
static struct volstream *vols_head;

static void volstream_add(struct volstream *vols) {
	pthread_rwlock_wrlock(&vols_lock);
	vols->next = vols_head;
	if (vols_head != NULL)
		vols_head->pprev = &(vols->next);
	vols->pprev = &vols_head;
	vols_head = vols;
	pthread_rwlock_unlock(&vols_lock);
}

static void volstream_del(struct volstream *vols) {
	struct volstream *next;
	pthread_rwlock_wrlock(&vols_lock);
	next = vols->next;
	if (next != NULL)
		next->pprev = vols->pprev;
	*(vols->pprev) = next;
}

static struct volstream *volstream_get(FILE *f) {
	struct volstream *scan;
	pthread_rwlock_rdlock(&vols_lock);
	for (scan = vols_head; scan != NULL; scan = scan->next)
		if (scan->f == f) break;

	if (scan != NULL)
		return scan;
	pthread_rwlock_unlock(&vols_lock);
	return NULL;
}

static void volstream_put(void) {
	pthread_rwlock_unlock(&vols_lock);
}

static ssize_t volstream_read(void *cookie, char *buf, size_t size) {
	struct volstream *vols = cookie;
	ssize_t ret_value;
	pthread_mutex_lock(&vols->mutex);
	if (vols->filepos + size <= vols->filelen)
		ret_value = size;
	else
		ret_value = vols->filelen - vols->filepos;
	if (ret_value > 0) {
		memcpy(buf, vols->buf + vols->filepos, ret_value);
		vols->filepos += ret_value;
	}
	pthread_mutex_unlock(&vols->mutex);
	return ret_value;
}

static inline void volstream_buf_expand(struct volstream *vols, size_t newfilesize) {
	if (newfilesize > vols->bufsize) {
		size_t newsize = vols->bufsize;
		while (newsize < newfilesize)
			newsize <<= 1;
		char *newbuf;
		newbuf = realloc(vols->buf, newsize);
		if (newbuf != NULL) {
			vols->buf = newbuf;
			vols->bufsize = newsize;
		}
	}
}

static inline void volstream_buf_shrink(struct volstream *vols, size_t newfilesize) {
	size_t newsize = vols->bufsize;
	while (newfilesize < newsize >> 1)
		newsize >>= 1;
	if (newsize < VOLSTREAM_MINBUFSIZE) newsize = VOLSTREAM_MINBUFSIZE;
	if (newsize < vols->bufsize) {
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
	pthread_mutex_lock(&vols->mutex);
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
	pthread_mutex_unlock(&vols->mutex);
	return ret_value;
}

static int volstream_seek(void *cookie, off64_t *offset, int whence) {
	struct volstream *vols = cookie;
	off64_t newpos;
	pthread_mutex_lock(&vols->mutex);
	switch (whence) {
		case SEEK_SET: newpos = *offset; break;
		case SEEK_CUR: newpos = vols->filepos + *offset; break;
		case SEEK_END: newpos = vols->filelen + *offset; break;
		default:       return errno = EINVAL, -1;
	}
	if (newpos < 0) {
		pthread_mutex_unlock(&vols->mutex);
		return errno = EINVAL, -1;
	} else {
		if (newpos > vols->filelen) {
			volstream_buf_expand(vols, newpos);
			if (newpos > vols->bufsize) {
				pthread_mutex_unlock(&vols->mutex);
				return errno = ENOMEM, -1;
			}
			if (newpos > vols->filelen) {
				memset(vols->buf + vols->filelen, 0, newpos - vols->filelen);
				vols->filelen = newpos;
			}
		}
		vols->filepos = newpos;
		*offset = newpos;
		pthread_mutex_unlock(&vols->mutex);
		return 0;
	}
}

static int volstream_close(void *cookie) {
	struct volstream *vols = cookie;
	volstream_del(vols);
	pthread_mutex_destroy(&vols->mutex);
	free(vols->buf);
	free(vols);
	volstream_put();
	return 0;
}

int volstream_trunc(FILE *f, size_t length) {
	struct volstream *vols;
	if (length < 0)
		return errno = EINVAL, -1;
	vols = volstream_get(f);
	if (vols == NULL)
		return errno = EBADF, -1;
	fflush(f);
	pthread_mutex_lock(&vols->mutex);
	if (length > vols->filelen) {
		volstream_buf_expand(vols, length);
		if (length > vols->bufsize) {
			pthread_mutex_unlock(&vols->mutex);
			volstream_put();
			return errno = ENOMEM, -1;
		}
		if (length > vols->filelen)
			memset(vols->buf + vols->filelen, 0, length - vols->filelen);
	}
	if (length < vols->filelen)
		volstream_buf_shrink(vols, length);
	vols->filelen = length;
	pthread_mutex_unlock(&vols->mutex);
	volstream_put();
	return 0;
}

int volstream_getbuf(FILE *f, void **buf, size_t *buflen) {
	struct volstream *vols = volstream_get(f);
	if (vols == NULL)
		return errno = EBADF, -1;
	fflush(f);
	pthread_mutex_lock(&vols->mutex);
	if (buf != NULL)
		*buf = vols->buf;
	if (buflen != NULL)
		*buflen = vols->filelen;
	pthread_mutex_unlock(&vols->mutex);
	volstream_put();
	return 0;
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
	vols->buf = malloc(VOLSTREAM_MINBUFSIZE);
	if (vols->buf == NULL)
		goto buferr;
	vols->bufsize = VOLSTREAM_MINBUFSIZE;
	vols->filelen = 0;
	vols->filepos = 0;
	volstream = fopencookie(vols, "r+", volstream_f);
	if (volstream == NULL)
		goto volstreamerr;
	vols->f = volstream;
	pthread_mutex_init(&vols->mutex, NULL);
	volstream_add(vols);
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
#include<unistd.h>
int main(int argc, char *argv[]) {
	FILE *f = volstream_open();
	int c;
	for (argv++; *argv; argv++) {
		fprintf(f, "%s\n", *argv);
		fflush(f);
	}
	fprintf(f,"FINE");
	volstream_trunc(f, 100);
	void *buf;
	ssize_t s;
	volstream_getbuf(f, &buf, &s);
	char *sbuf = buf;
	for (ssize_t i=0; i<s; i++)
		putchar(sbuf[i] == 0 ? '.' : sbuf[i]);
	printf("\n");
	fseek(f, 0, SEEK_SET);
	while ((c = getc(f)) != EOF)
		putchar(c == 0 ? '.' : c);
	printf("\n");
	fclose(f);
}
#endif
