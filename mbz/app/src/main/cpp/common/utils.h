#ifndef MBZ_UTILITIES_H
#define MBZ_UTILITIES_H

#include <sys/types.h>

#include <android/log.h>

#define ERROR_PRINT(tag, fmt, ...) (__android_log_print(ANDROID_LOG_ERROR, tag, fmt, ##__VA_ARGS__))

#ifdef DEBUG
#define DEBUG_PRINT(tag, fmt, ...) (__android_log_print(ANDROID_LOG_DEBUG, tag, fmt, ##__VA_ARGS__))
#else
#define DEBUG_PRINT(tag, fmt, ...)
#endif

namespace common {
  void printSyscallError(const char *tag, const char *fmt);
  bool writeAllFd(int fd, const uint8_t *buf, size_t len);
  int readline(int fd, char *buf, int len);
}


#endif //MBZ_UTILITIES_H
