/**
 *  @file
 *  @brief      Define some functions everyone expects.
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

#ifndef PORTABILITY_H_2L1YN201
#define PORTABILITY_H_2L1YN201

#include "umundo/core/Common.h"

#if defined(NO_STRNDUP)
char* strndup (const char* s, size_t n);
#endif

#if defined(NO_STRNLEN)
size_t strnlen(const char* s, size_t n);
#endif

#ifdef ANDROID
#include <wchar.h>
namespace std {
// prevent error: 'wcslen' is not a member of 'std'
using ::wcslen;
}
#endif

#ifdef _WIN32
#include <stdarg.h>

#define setenv(key, value, override) _putenv_s(key, value)

#define strncasecmp(x,y,z) _strnicmp(x,y,z)

#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef isatty
#define isatty _isatty
#endif

#ifndef strdup
#define strdup _strdup
#endif

#ifndef va_copy
#define va_copy(d,s) ((d) = (s))
#endif

int vasprintf(char **ret, const char *format, va_list args);
int asprintf(char **ret, const char *format, ...);

#endif

#endif /* end of include guard: PORTABILITY_H_2L1YN201 */
