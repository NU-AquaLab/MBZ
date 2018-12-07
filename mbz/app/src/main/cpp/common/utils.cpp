#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

#define TAG "Utilities"

namespace common {
  void printSyscallError(const char *tag, const char *fmt) {
    char err[100];
    strerror_r(errno, err, sizeof(err));

    ERROR_PRINT(tag, fmt, err);
  }

  bool writeAllFd(int fd, const uint8_t *buf, size_t len) {
    size_t written = 0;
    int rc;

    while (written < len) {
      rc = (int) write(fd, buf + written, len - written);

      if (rc < 0) {
        printSyscallError(TAG, "Error writing fd: %s");
        return false;
      }

      written += rc;
    }

    return true;
  }

  int readline(int fd, char *buf, int len) {
    int rc;
    int n = 0;

    while (1) {
      rc = (int) read(fd, buf+n, 1);

      if (rc < 0) {
        printSyscallError(TAG, "Unable to read from file: %s");
        return -1;
      }
      else if (rc == 0 && n == 0) {
        return 0;
      }
      else if (rc == 0) {
        ERROR_PRINT(TAG, "EOF reached before newline.");
        return -1;
      }
      else {
        n += rc;

        if (buf[n-1] == '\n') {
          buf[n-1] = '\0';
          break;
        }
        else if (n >= len) {
          ERROR_PRINT(TAG, "Buffer too small to read newline.");
          return -1;
        }
      }
    }

    return n;
  }
}
