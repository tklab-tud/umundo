/**
 *  @file
 *  @brief      Logging
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

#ifndef DEBUG_H_Z6YNJLCS
#define DEBUG_H_Z6YNJLCS

#include "umundo/core/Common.h"
#include "umundo/core/thread/Thread.h"

#include <stdarg.h> ///< variadic functions

/**
 * Log messages with a given priority, disable per compilation unit by defining
 * NO_DEBUG_MSGS before including any headers.
 *
 */
#ifdef NO_DEBUG_MSGS
#	define UM_LOG_ERR(fmt, ...) ((void)0)
#	define UM_LOG_WARN(fmt, ...) ((void)0)
#	define UM_LOG_INFO(fmt, ...) ((void)0)
#	define UM_LOG_DEBUG(fmt, ...) ((void)0)
#else
#	define UM_LOG_ERR(fmt, ...) umundo::Debug::logMsg(0, fmt, __FILE__, __LINE__,  ##__VA_ARGS__)
#	define UM_LOG_WARN(fmt, ...) umundo::Debug::logMsg(1, fmt, __FILE__, __LINE__,  ##__VA_ARGS__)
#	define UM_LOG_INFO(fmt, ...) umundo::Debug::logMsg(2, fmt, __FILE__, __LINE__,  ##__VA_ARGS__)
#	define UM_LOG_DEBUG(fmt, ...) umundo::Debug::logMsg(3, fmt, __FILE__, __LINE__,  ##__VA_ARGS__)
#endif

#ifdef ENABLE_TRACING
#define UM_TRACE(fmt, ...) umundo::Trace trace##__LINE__(fmt, __FILE__, __LINE__,  ##__VA_ARGS__)
#else
#define UM_TRACE(fmt, ...) ((void)0)
#endif

#ifdef __GNUC__
#ifndef ANDROID
#define HAVE_EXECINFO
#endif
#endif

namespace umundo {

/**
 * Macros and static functions used for debugging.
 *
 * All umundo logging is to be done using one of the following macros:
 * - #UM_LOG_ERR(fmt, ...)
 * - #UM_LOG_WARN(fmt, ...)
 * - #UM_LOG_INFO(fmt, ...)
 * - #UM_LOG_DEBUG(fmt, ...)
 *
 * These macros will take care of calling Debug::logMsg() for you. By using macros, we can simply remove them in release builds.
 * The macros will return a boolean to allow logging in lazy evaluated expressions:
 *
 * trueForSuccess() || UM_LOG_WARN("Failed to succeed");
 */
class UMUNDO_API Debug {
public:
	static const char* relFileName(const char* filename);
	static bool logMsg(int lvl, const char* fmt, const char* filename, const int line, ...);
#ifdef HAVE_EXECINFO
	static void abortWithStackTraceOnSignal(int sig);
	static void stackTraceSigHandler(int sig);
#else
	// noop with non-gcc compilers
	static void abortWithStackTraceOnSignal(int sig) {};
	static void stackTraceSigHandler(int sig) {};
#endif
};


class UMUNDO_API Trace {
public:
	Trace(const char* fmt, const char* filename, const int line, ...);
	~Trace();
protected:
	static std::map<int, int> _threadNesting;
	std::string _msg;
	std::string _file;
	int _threadId;
	int _line;
};

}

#endif /* end of include guard: DEBUG_H_Z6YNJLCS */
