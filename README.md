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

## v.0.2

It is backwards compatible to v.0.1.

New functions:
```C
       struct volstream;
       FILE *volstream_openv(struct volstream **vols);
       int volstream_trunc(struct volstream *vols, size_tlength);
       void *volstream_getbuf(struct volstream *vols);
       size_t volstream_getsize(struct volstream *vols);
```

The function `volstream_openv` has the same effect of `volstream_open` but it
stores in _vols_ a pointer that can be later used for `volstream_trunc`, `volstream_getbuf`
and `volstream_getsize`.

`volstream_trunc` truncates the buffer to the requested _length_. If the current size
of the buffer is larger than _length_ the extra data is lost. If the buffer is shorter
it is extended and the extended part is filled with null bytes.

`volstream_getbuf` and `volstream_getsize` return the current address and size of the buffer,
respectively. These values remain valid only as long as the caller performs no further output
on the stream or the stream is closed.

`fflush` is required before `volstream_trunc`, `volstream_getbuf` and `volstream_getsize` to
flush the stream buffers.

The following example has the same effect of the one above but it rereads the arguments as a 
memory buffer.

```
#include <stdio.h>
#include <unistd.h>
#include <volatilestream.h>

int main(int argc, char *argv[]) {
  struct volstream *vols;
  FILE *f = volstream_openv(&vols);
  int c;
  for (argv++; *argv; argv++) {
    fprintf(f, "%s\n", *argv);
  }
  fflush(f);
  ssize_t s = volstream_getsize(vols);
  char *buf = volstream_getbuf(vols);
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
