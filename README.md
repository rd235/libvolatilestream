# libvolatilestream
volatile stream = stdio FILE\* stream as a temporary dynamically allocated (and deallocated) memory buffer

The `volstream_open` function opens a stdio stream as a temporary memory buffer.
The buffer is dynamically allocated, grows as needed and it is automatically
deallocated when the stream is closed.

```C
FILE *volstream_open(void);
```

The following example writes all the command arguments in a volatile stream,
		then it rereads the volatile stream one byte at a time 

```C
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

## v.1.0

New functions:
```C
       int volstream_trunc(FILE *f, size_t length);
       int volstream_getbuf(FILE *f, void **buf, size_t *buflen);
```

`volstream_trunc` truncates the buffer to the requested _length_. If the current size
of the buffer is larger than _length_ the extra data is lost. If the buffer is shorter
it is extended and the extended part is filled with null bytes.

`volstream_getbuf` writes the current address and size of the buffer in `*buf` and `*buflen`
respectively.
These values remain valid only as long as the caller performs no further output
on the stream or the stream is closed.

`volstream_getbuf` returns 0  or -1 if an error occurred.  In the event
of an error, errno is set to indicate the error.

The following example has the same effect of the one above but it rereads the arguments as a
memory buffer.

```
#include <stdio.h>
#include <unistd.h>
#include <volatilestream.h>

int main(int argc, char *argv[]) {
  FILE *f = volstream_open();
  int c;
  for (argv++; *argv; argv++) {
    fprintf(f, "%s\n", *argv);
  }
  ssize_t s;
  void *buf;
  volstream_getbuf(f, &buf, &s);
  write(STDOUT_FILENO, buf, s);
  fclose(f);
}
```

## Install

libvolatilestream uses cmake. A standard installation procedure is:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

Use ```-l volatilestream``` as a gcc/ld option to link the library.

## Uninstall

In the build directory type:
```
$ sudo make uninstall
```
