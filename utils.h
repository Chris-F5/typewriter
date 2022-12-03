#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#define OR ? (errno = 0) : (errno = errno ? errno : -1); if (errno)

#endif
