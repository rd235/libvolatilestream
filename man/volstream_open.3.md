<!--
.\" Copyright (C) 2019 VirtualSquare. Project Leader: Renzo Davoli
.\"
.\" This is free documentation; you can redistribute it and/or
.\" modify it under the terms of the GNU General Public License,
.\" as published by the Free Software Foundation, either version 2
.\" of the License, or (at your option) any later version.
.\"
.\" The GNU General Public License's references to "object code"
.\" and "executables" are to be interpreted as the output of any
.\" document formatting or typesetting system, including
.\" intermediate and printed output.
.\"
.\" This manual is distributed in the hope that it will be useful,
.\" but WITHOUT ANY WARRANTY; without even the implied warranty of
.\" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
.\" GNU General Public License for more details.
.\"
.\" You should have received a copy of the GNU General Public
.\" License along with this manual; if not, write to the Free
.\" Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
.\" MA 02110-1301 USA.
.\"
-->
# NAME

`volstream_open` -- create a FILE* stream as a volatile stream buffer

# SYNOPSIS

`#include <volatilestream.h>`

`FILE *volstream_open(void);`

`struct volstream;`

`int volstream_trunc(FILE *`_f_`, size_t ` _length_`);`

`int volstream_getbuf(FILE *`_f_`, void **`_buf_`, size_t *`_buflen_`);`

# DESCRIPTION

A volatile stream is a stdio FILE* stream as a temporary dynamically allocated
(and deallocated) memory buffer.

The `volstream_open` function opens a stdio stream as a temporary memory
buffer. The buffer is dynamically allocated, grows as needed and it is
automatically deallocated when the stream is closed.

`volstream_trunc` truncates the buffer to the requested _length_. If the current size
of the buffer is larger than _length_ the extra data is lost. If the buffer is shorter
it is extended and the extended part is filled with null bytes.

`volstream_getbuf` writes the current address and size of the buffer in `*buf` and `*buflen`
respectively.
These values remain valid only as long as the caller performs no further output
on the stream or the stream is closed.

# RETURN VALUE

Upon successful completion `volstream_open` returns a FILE pointer.
Otherwise, NULL is returned and errno is set to indicate the error.

`volstream_trunc` and `volstream_getbuf` return 0  or -1 if an error occurred.  In the event
of an error, errno is set to indicate the error.

# EXAMPLES

The following example writes all the command arguments in a volatile stream,
then it rereads the volatile stream one byte at a time:

```
#include *stdio.h*
#include *volatilestream.h*

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
```

The following example has the same effect but it rereads the arguments as a memory buffer.

```
#include *stdio.h*
#include *unistd.h*
#include *volatilestream.h*

int main(int argc, char *argv[]) {
  FILE *f = volstream_open();
  int c;
  for (argv++; *argv; argv++) {
    fprintf(f, "%s\n", *argv);
  }
  fflush(f);
  ssize_t s;
  void *buf;
  volstream_getbuf(f, &buf, &s);
  write(STDOUT_FILENO, buf, s);
  fclose(f);
}
```

# AUTHOR
VirtualSquare. Project leader: Renzo Davoli.
