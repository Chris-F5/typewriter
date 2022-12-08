#ifndef TW_UTILS_H
#define TW_UTILS_H

#include <errno.h>
#define OR ? (errno = 0) : (errno = errno ? errno : -1); if (errno)

#endif
