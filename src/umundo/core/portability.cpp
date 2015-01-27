/**
 *  @file
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

#include "umundo/core/portability.h"
#include "umundo/config.h"

// there is no no strndup in Mac OSX 10.5
#if defined(NO_STRNDUP)
char* strndup (const char* s, size_t n) {
	char* result;
	size_t len = strlen (s);

	if (n < len)
		len = n;

	result = (char*)malloc (len + 1);
	if (!result)
		return 0;

	result[len] = '\0';
	return (char*)memcpy (result, s, len);
}
#endif

#if defined(NO_STRNLEN)
size_t strnlen(const char* s, size_t n) {
	const char* end = (const char*)memchr(s, 0, n);
	return end ? (size_t)(end - s) : n;
}
#endif

#ifdef ANDROID
// size_t std::wcslen(const wchar_t *) {
// 	return 1;
// }
#endif

#ifdef WIN32
// Taken from:
// http://stackoverflow.com/questions/4899221/substitute-or-workaround-for-asprintf-on-aix
int vasprintf(char **ret, const char *format, va_list args) {
	va_list copy;
	va_copy(copy, args);

	/* Make sure it is determinate, despite manuals indicating otherwise */
	*ret = 0;

	int count = vsnprintf(NULL, 0, format, args);
	if (count >= 0) {
		char* buffer = (char*)malloc(count + 1);
		if (buffer != NULL) {
			count = vsnprintf(buffer, count + 1, format, copy);
			if (count < 0)
				free(buffer);
			else
				*ret = buffer;
		}
	}
	va_end(args);  // Each va_start() or va_copy() needs a va_end()

	return count;
}

int asprintf(char **ret, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int count = vasprintf(ret, format, args);
	va_end(args);
	return(count);
}

#endif
