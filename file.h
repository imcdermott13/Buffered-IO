//
// file.h
//
// Implement the C standard I/O library in C++ style.
//
// Author: Morris Bernstein
// Copyright 2019, Systems Deployment, LLC.
//
// Modified by: Ian McDermott

#if !defined(FILE_H)
#define FILE_H

#include <cstddef>
#include <exception>


class File {
public:
  class File_Exception: public std::exception {
  };

  enum BufferMode {
    NO_BUFFER,
    LINE_BUFFER,
    FULL_BUFFER
  };

  // Use lowercase because system header files #define the uppercase
  // names.
  enum Whence {
    seek_set,
    seek_cur,
    seek_end
  };

  static const int bufsiz = 8192;
  static const int eof = -1;

  // Open a file.
  // Mode can be "r", "r+", "w", "w+",
  // Modes "a", and "a+" are unsupported.
  // Use default buffering: FULL_BUFFER.
  File(const char *name, const char *mode = "r");

  // Close the file.  Make sure any buffered data is written to disk,
  // and free the buffer if there is one.
  ~File();

  // Return non-zero value if the file is in an error state.
  // When the file is in an error state, I/O operations are not
  // performed and appropriate values are returned.
  int ferror();

  // Return true if end-of-file is reached.  This is not an error
  // condition.  Reading past eof is an error.
  bool feof();

  // Add a user-defined buffer and set the buffering mode.  If
  // non-null, the buffer must have been created by malloc and will be
  // freed by the destructor (or by another call to setvbuf).
  // LINE_BUFFER mode need not be supported.
  int setvbuf(char *buf, BufferMode mode, size_t size);

  // If data is buffered for writing, write the buffered data to
  // disk.  Reset the buffer to empty. Reset the file pointer so it
  // behaves the way the user would expect.
  int fflush();

  // If the amount of data to be read or written exceeds the buffer,
  // avoid double-buffering by reading/writing data directly to/from
  // the source/destination.
  size_t fread(void *ptr, size_t size, size_t nmemb);
  size_t fwrite(const void *ptr, size_t size, size_t nmemb);

  int fgetc();
  int fputc(int c);

  char *fgets(char *s, int size);
  int fputs(const char *str);

  // Flush any buffered data and reset the file pointer.
  int fseek(long offset, Whence whence);

  // Stripped-down version: only implements %d and %s format codes.
  int fprintf(const char *format, ...);

private:
  char *buf;
  size_t bufAt = 0;
  size_t bufEnd = 0;
  BufferMode bmode = FULL_BUFFER;
  char fmode;
  char lastAct = '0';
  int fd;
  int err = 0;
  bool end = false;

  // Disallow copy & assignment.
  File(File const&) = delete;
  File& operator=(File const&) = delete;
};


#endif
