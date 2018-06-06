# libvolatilestream
volatile stream = stdio FILE\* stream as a temporary dynamically allocated (and deallocated) memory buffer

The ```volstream_open``` function opens a stdio stream as a temporary memory buffer.
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

## Install

libvolatilestream uses cmake. A standard installation procedure is:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

Use ```-l volatilestream``` as a gcc/ld option to link the library.
