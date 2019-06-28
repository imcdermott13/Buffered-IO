//
// file.cc
//
// Implement the C standard I/O library in C++ style.
//
// Skeleton provided by: Morris Bernstein
// Copyright 2019, Systems Deployment, LLC.
//
// Modified by: Ian McDermott


#include "file.h"

#include <fcntl.h>	// open
#include <unistd.h>	// read
#include <sys/types.h>		// read
#include <stdlib.h>     // malloc, free
#include <cassert>
#include <cstdarg>

static char *itoa(int, char*);

// Opens the file in the correct mode and allocates the buffer
File::File(const char *name, const char *mode) {
  if (mode[0] == 'r' && mode[1] == '\0') {
    this->fd = open(name, O_RDONLY);
    this->fmode = 'r';
  } else if (mode[0] == 'w' && mode[1] == '\0') {
    this->fd = open(name, O_WRONLY);
    this->fmode = 'w';
  } else if ((mode[0] == 'r' || mode[0] == 'w') && mode[1] == '+') {
    this->fd = open(name, O_RDWR);
    this->fmode = '+';
  }
  if (this->fd < 0)
    throw "Open failure";
  this->buf = reinterpret_cast<char*>(malloc(bufsiz));
}

// Frees the buffer and closes the file
File::~File() {
  try {
    this->fflush();
    free(this->buf);
    int cls = close(this->fd);
    if (cls == -1)
      throw "Close failure";
  }
  catch (...) {
    assert(0);
  }
}


int File::ferror() {
  return this->err;
}


bool File::feof() {
  return this->end;
}


int File::setvbuf(char *buf, BufferMode mode, size_t size) {
  assert(0);			// Not implemented.
}


int File::fflush() {
  // If the last action was writing, then the buffer needs to be written to file
  if (lastAct == 'w') {
    if (write(this->fd, this->buf, this->bufAt) < 0)
      return eof;
  } else if (lastAct == 'r') {
    if (lseek(this->fd, this->bufAt - this->bufEnd, SEEK_CUR) == (off_t)-1) {
      this->err = -4;
      return eof;
    }
  }
  this->bufAt = 0;
  this->bufEnd = 0;
  this->lastAct = '0';
  return 0;
}


size_t File::fread(void *ptr, size_t size, size_t nmemb) {
  if (this->fmode == 'w') return eof; // stops if file is write only
  if (this->lastAct == 'w') {
    if (this->fflush() != 0) // flush if switching between I/O
      return eof;
  }
  
  size_t ptrAt = 0;
  if (this->lastAct == 'r') {
    // If going to reach end of buffer, empties buffer into ptr
    if (this->bufAt + size * nmemb > this->bufEnd) {
      while (bufAt < bufEnd)
        *(((char *)ptr) + ptrAt++) = *(this->buf + this->bufAt++);
      if (this->fflush() != 0) {
        this->bufAt -= ptrAt;
        return eof;
      }
    }
  }
  if (this->lastAct == '0') { // If no action yet or fflush was last action
    // If buffer isn't large enough, read directly into ptr
    if (size * nmemb - ptrAt > bufsiz) {
      size_t bytes_read = read(this->fd, (void *)((char *)ptr + ptrAt),
                               nmemb * size);
      if (bytes_read < 0) {
        this->err = -3;
        return eof;
      }
      if (bytes_read < size * nmemb - ptrAt) this->end = true;
      return bytes_read + ptrAt;
    } else { // If buffer is large enough, read into buffer first
      this->bufEnd = read(this->fd, this->buf, bufsiz);
      if (this->bufEnd < 0) {
        this->err = -2;
	return eof;
      }
      if (this->bufEnd == 0) {
        this->end = true;
        return ptrAt;
      }
    }
  } 
  this->lastAct = 'r'; // sets last action to 'r' to check for I/O switch

  while (ptrAt < size * nmemb) {
    if (this->bufAt == this->bufEnd && this->bufEnd < bufsiz) {
      this->end = true;
      break;
    }
    *(((char *)ptr) + ptrAt++) = *(this->buf + this->bufAt++);
  }
  return ptrAt;
}


size_t File::fwrite(const void *ptr, size_t size, size_t nmemb) {
  if (this->fmode == 'r') return eof; // stops if file is read only
  if (this->lastAct == 'r') { 
    if (this->fflush() != 0) // flushes if switching between I/O
      return eof;
  }
  this->lastAct = 'w'; // sets last action to 'w' to check for I/O switch

  size_t bytes_written = 0;
  if (this->bufAt + size * nmemb > bufsiz) { // checks if write fits in buffer
    if (this->fflush() != 0) return eof;
    if (size * nmemb > bufsiz) {
      bytes_written = write(this->fd, ptr, size * nmemb);
      if (bytes_written < 0) {
        this->err = -1;
        return eof;
      }
    }
  }
  if (bytes_written == 0) { // only != 0 if the write won't fit in the buffer
    for (; bytes_written < size * nmemb; bytes_written++) {
      *(this->buf + this->bufAt++) = ((char *)ptr)[bytes_written];
    }
  }
  return bytes_written;
}


int File::fgetc() {
  char temp[1] = {'\0'};
  // checks if file is write only and for I/O switch inside fread call
  if (this->fread(temp, 1, 1) < 0) return eof;
  return (int)(*temp);
}


int File::fputc(int c) {
  char a[1] = {(char)c};
  // checks if file is read only and for I/O switch inside fwrite call
  if (this->fwrite((void *)a, 1, 1) < 0) return eof;
  return c;
}


char *File::fgets(char *s, int size) {
  if (this->fmode == 'w') return NULL; // stops if file is write only
  if (this->lastAct == 'w') {
    if (this->fflush() != 0) // flushes if switching between I/O
      return NULL;
  }

  int file_offset = this->bufAt;
  int overflow = 0;
  if (this->bufAt == this->bufEnd) overflow++;
  int temp;
  int sAt = 0;
  
  // reads individual chars until reaching size total chars, a '\n' char, or eof
  for (int i = 0; i < size; i++) {
    temp = fgetc();
    if (this->bufAt == this->bufEnd)
      overflow++; // count times end of buffer reached
    if (temp == eof) {
      if (!this->feof()) {
        // If an error occurs, empty buffer and reset file to original state
	this->bufAt = 0;
	this->bufEnd = 0;
	this->lastAct = '0';
        if (lseek(this->fd, file_offset - this->bufEnd - (overflow * bufsiz),
                  SEEK_CUR) == (off_t)-1)
          throw "Reposition failure";
        return NULL;
      } else break;
    } else *(s + sAt++) = (char)temp;
    if (temp == '\n') break;
  }
  return s;
}


int File::fputs(const char *str) {
  if (this->fmode == 'r') return -1; // stops if file is read only
  // checks if I/O switchws in fwrite call
  int size = 0;
  while (str[size] != '\0') size++; // determines the size of str
  return this->fwrite((void *)str, 1, size);
}


int File::fseek(long offset, Whence whence) {
  this->fflush();
  int where;
  if (whence == seek_set) where = SEEK_SET;
  else if (whence == seek_cur) where = SEEK_CUR;
  else if (whence == seek_end) where = SEEK_END;
  else return -2; // if (somehow) whence isn't set correctly
  if (lseek(this->fd, offset, where) == (off_t)-1) return -1;
  this->end = false;
  return 0;
}


// log10(2**64) (~ 20) + sign character + trailing NUL byte ('\0')
// Rounded up to word size.
static const int ITOA_BUFSIZE = 32;

static char *itoa(int i, char a[ITOA_BUFSIZE]) {
  char * p = a + (ITOA_BUFSIZE - 1);
  *p = '\0';			// Trailing NUL byte: end-of-string.
  if (i == 0) {
    *--p = '0';
    return p;
  }
  bool negative = (i < 0);
  if (negative) {
    i = -i;
  }
  for (; i > 0; i = i / 10) {
    *--p = '0' + (i % 10);
  }
  if (negative) {
    *--p = '-';
  }
  return p;
}


// Stripped-down version: only implements %d, %s, and %% format codes.
int File::fprintf(const char *format, ...) {
  int n = 0;			// Number of characters printed.
  va_list arg_list;
  va_start(arg_list, format);
  for (const char *p = format; *p != '\0'; p++) {
    if (*p != '%') {
      if (fputc(*p) < 0) {
	return -1;
      }
      n++;
      continue;
    }
    switch(*++p) {
    case 's':
      {
	char *s = va_arg(arg_list, char *);
	for (;*s != '\0'; s++) {
	  if (fputc(*s) < 0) {
	    return -1;
	  }
	  n++;
	}
      }
      break;
    case 'd':
      {
	int i = va_arg(arg_list, int);
	char sbuf[ITOA_BUFSIZE];
	char *s = itoa(i, sbuf);
	for (;*s != '\0'; s++) {
	  if (fputc(*s) < 0) {
	    return -1;
	  }
	  n++;
	}
      }
      break;
    default:
      if (fputc(*p) < 0) {
	return -1;
      }
      n++;
    }
  }
  va_end(arg_list);
  return n;
}
