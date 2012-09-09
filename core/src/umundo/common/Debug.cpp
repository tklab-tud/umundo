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

#include "umundo/common/Debug.h"
#include "umundo/thread/Thread.h"
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "umundo/config.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef WIN32
#include <io.h>
#endif

#define RESET       0
#define BRIGHT      1
#define FAINT       2
#define ITALIC      3
#define UNDERLINE   4
#define BLINK_SLOW  5
#define BLINK_FAST  6
#define INVERT      7
#define HIDDEN      8
#define STRIKED     9

#define BLACK       0
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define MAGENTA     5
#define CYAN        6
#define	WHITE       7

namespace umundo {

// required for padding
static int longestFilename = 0;
static int longestLineNumber = 0;
static const char* lastLogDomain = NULL;
static time_t lastTime = 0;

// log levels can be overwritten per environment
static bool determinedLogLevels = false;
static int logLevelCommon = -1;
static int logLevelNet = -1;
static int logLevelDisc = -1;
static int logLevelS11n = -1;

static int useColors = -1;
static int useThreadId = -1;
static int useMSinLog = -1;

const char* Debug::relFileName(const char* filename) {
	const char* relPath = filename;
	if(strstr(filename, PROJECT_SOURCE_DIR)) {
		relPath = filename + strlen(PROJECT_SOURCE_DIR) + 1;
	}
	return relPath;
}

bool Debug::logMsg(int lvl, const char* fmt, const char* filename, const int line, ...) {
	// try to shorten filename
	filename = relFileName(filename);
	char* pathSepPos = (char*)filename;
	const char* logDomain = NULL;
	bool logDomainChanged = false;

	// determine actual log levels once per program start
	if (!determinedLogLevels) {
		// if UMUNDO_LOGLEVEL is defined in environment, it overwrites loglevels from build time
		logLevelCommon = (getenv("UMUNDO_LOGLEVEL") != NULL ? atoi(getenv("UMUNDO_LOGLEVEL")) : LOGLEVEL_COMMON);
		logLevelNet    = (getenv("UMUNDO_LOGLEVEL") != NULL ? atoi(getenv("UMUNDO_LOGLEVEL")) : LOGLEVEL_NET);
		logLevelDisc   = (getenv("UMUNDO_LOGLEVEL") != NULL ? atoi(getenv("UMUNDO_LOGLEVEL")) : LOGLEVEL_DISC);
		logLevelS11n   = (getenv("UMUNDO_LOGLEVEL") != NULL ? atoi(getenv("UMUNDO_LOGLEVEL")) : LOGLEVEL_S11N);

		// if specific loglevel is defined in environment, it takes precedence over everything
		if (getenv("UMUNDO_LOGLEVEL_COMMON") != NULL) logLevelCommon = atoi(getenv("UMUNDO_LOGLEVEL_COMMON"));
		if (getenv("UMUNDO_LOGLEVEL_NET") != NULL)    logLevelNet = atoi(getenv("UMUNDO_LOGLEVEL_NET"));
		if (getenv("UMUNDO_LOGLEVEL_DISC") != NULL)   logLevelDisc = atoi(getenv("UMUNDO_LOGLEVEL_DISC"));
		if (getenv("UMUNDO_LOGLEVEL_S11N") != NULL)   logLevelS11n = atoi(getenv("UMUNDO_LOGLEVEL_S11N"));

		determinedLogLevels = true;
	}

	// determine whether we want colored output
	if (useColors < 0) {
		useColors = 0;
		if (getenv("UMUNDO_LOGCOLORS") != NULL && (strcmp(getenv("UMUNDO_LOGCOLORS"), "NO") == 0 || strcmp(getenv("UMUNDO_LOGCOLORS"), "0") == 0)) {
			// color explicitly disabled per environment
			useColors = 0;
		} else {
			// even if we want colors, check if terminal supports them
#ifdef WIN32
			CONSOLE_SCREEN_BUFFER_INFO sbi;
			if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi)) {
				useColors = 1;
			}
#else
			if (isatty(1)) {
				char* term = getenv("TERM");
				if ((term != NULL) && (strncmp(term, "xterm", 5) == 0 || strcmp(term, "xterm") == 0)) {
					useColors = 1;
				}
				char* unbuffIO = getenv("NSUnbufferedIO");
				if ((unbuffIO != NULL) && (strncmp(unbuffIO, "YES", 3) == 0)) {
					useColors = 0;
				}
			}
#endif
		}
	}

	// check whether we want to use the thread id in log messages
	// this caused some problems at Thread destruction
	if (useThreadId < 0) {
		if (getenv("UMUNDO_THREADID_IN_LOG") != NULL && strcmp(getenv("UMUNDO_THREADID_IN_LOG"), "ON") == 0) {
			useThreadId = 1;
		} else {
			useThreadId = 0;
		}
	}

	if (useMSinLog < 0) {
		if (getenv("UMUNDO_MILLISEC_IN_LOG") != NULL && strcmp(getenv("UMUNDO_MILLISEC_IN_LOG"), "ON") == 0) {
			useMSinLog = 1;
		} else {
			useMSinLog = 0;
		}
	}

	// check log domain and whether we will log at all
	while((pathSepPos = strchr(pathSepPos + 1, PATH_SEPERATOR))) {
		if (strncmp(pathSepPos + 1, "common", 6) == 0) {
			if (lvl > logLevelCommon)
				return false;
			logDomain = "common";
		} else if (strncmp(pathSepPos + 1, "connection", 10) == 0) {
			if (lvl > logLevelNet)
				return false;
			logDomain = "connection";
		} else if (strncmp(pathSepPos + 1, "discovery", 9) == 0) {
			if (lvl > logLevelDisc)
				return false;
			logDomain = "discovery";
		} else if (strncmp(pathSepPos + 1, "s11n", 4) == 0) {
			if (lvl > logLevelS11n)
				return false;
			logDomain = "s11n";
		}
		filename = pathSepPos + 1;
	}

	// tread unknown domains as common
	if (logDomain == NULL) {
		if (lvl > logLevelCommon)
			return false;
		logDomain = "common";
	}

	// only color first line of a new log domain
	if (lastLogDomain == NULL || strcmp(lastLogDomain, logDomain) != 0) {
		lastLogDomain = logDomain;
		logDomainChanged = true;
	}

	// determine length of filename:line for padding
	int lineNumberLength = (int)ceil(log((float)line + 1) / log((float)10));
	if ((int)strlen(filename) + lineNumberLength > longestFilename) {
		longestFilename = strlen(filename) + lineNumberLength;
		longestLineNumber = lineNumberLength;
	}

	const char* severity = NULL;
	if (lvl == 0) severity = "ERROR";
	if (lvl == 1) severity = "WARNING";
	if (lvl == 2) severity = "INFO";
	if (lvl == 3) severity = "DEBUG";

	char* padding = (char*)malloc((longestFilename - (strlen(filename) + lineNumberLength)) + 1);
	assert(padding != NULL);
	padding[(longestFilename - (strlen(filename) + lineNumberLength))] = 0;
	memset(padding, ' ', longestFilename - (strlen(filename) + lineNumberLength));

	char* message;
	va_list args;
	va_start(args, line);
	vasprintf(&message, fmt, args);
	va_end(args);

	// get current thread id
	int threadId = -1;
	if (useThreadId && strcmp(filename, "Thread.cpp") != 0)
		threadId = Thread::getThreadId();

	// timestamp
	char timeStr[9] = "        ";  // space for "HH:MM:SS\0"
	if (useMSinLog) {
		int64_t currTime = Thread::getTimeStampMs();
		snprintf(timeStr, 8, "%ld", (long)(currTime % 100000000));
	} else {
		time_t current_time;
		struct tm * time_info;
		time(&current_time);
		if (lastTime != current_time) {
			time_info = localtime(&current_time);
			strftime(timeStr, sizeof(timeStr), "%H:%M:%S", time_info);
			lastTime = current_time;
		}
	}

#ifdef ANDROID
	__android_log_print(ANDROID_LOG_VERBOSE, logDomain, "%02d|%s:%d: %s %s\n", threadId, filename, line, severity, message);
#else
	if (useColors) {
#ifdef WIN32
		HANDLE hConsole;
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		int attribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
		if (logDomain != NULL && logDomainChanged) {
			if (strcmp(logDomain, "common") == 0)      attribute |= BACKGROUND_BLUE | BACKGROUND_GREEN;
			if (strcmp(logDomain, "connection") == 0)  attribute |= BACKGROUND_RED | BACKGROUND_BLUE;
			if (strcmp(logDomain, "discovery") == 0)   attribute |= BACKGROUND_GREEN | BACKGROUND_RED;
			if (strcmp(logDomain, "s11n") == 0)        attribute |= BACKGROUND_BLUE;
		} else {
			if (lvl == 1) attribute |= FOREGROUND_INTENSITY;
			if (lvl == 2) attribute |= COMMON_LVB_UNDERSCORE;
			if (lvl == 3) attribute |= 0;
		}

		SetConsoleTextAttribute(hConsole, attribute);

		printf("%s %02d|%s:%d:%s %s %s\n", timeStr, threadId, filename, line, padding, severity, message);

#else

		int effect = 0;
		int foreground = BLACK;
		int background = WHITE;

		if (logDomain != NULL && logDomainChanged) {
			if (strcmp(logDomain, "common") == 0)      background = CYAN;
			if (strcmp(logDomain, "connection") == 0)  background = MAGENTA;
			if (strcmp(logDomain, "discovery") == 0)   background = YELLOW;
			if (strcmp(logDomain, "s11n") == 0)        background = BLUE;
		} else {
			if (lvl == 1) effect = BRIGHT;
			if (lvl == 2) effect = UNDERLINE;
			if (lvl == 3) effect = 0;
		}
		// errors are white on red in any case
		if (lvl == 0) {
			effect = RESET;
			background = RED;
			foreground = WHITE;
		}

		printf("\e[%d;%d;%dm%s %02d|%s:%d:%s %s %s\e[0m\n", effect, foreground + 30, background + 40, timeStr, threadId, filename, line, padding, severity, message );
#endif
	} else {
		printf("%s %02d|%s:%d:%s %s %s\n", timeStr, threadId, filename, line, padding, severity, message);
	}
	fflush(stdout);
#endif
	free(message);
	free(padding);
	return true;
}

#ifdef HAVE_EXECINFO
#include <execinfo.h>
#include <signal.h>

void Debug::abortWithStackTraceOnSignal(int sig) {
//	signal(sig, stackTraceSigHandler);
}

void Debug::stackTraceSigHandler(int sig) {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(1);
}

#endif

Mutex Traceable::_mutex;
std::map<std::string, boost::weak_ptr<std::ofstream> > Traceable::_files;

Traceable::Traceable() {
}

Traceable::~Traceable() {
	ScopeLock lock(_mutex);
	if (_traceFile) {
		_traceFile->flush();
	}
}

bool Traceable::setTraceFile(const std::string& filename) {
	_traceFileName = filename;
	ScopeLock lock(_mutex);
	if (_files.find(_traceFileName) == _files.end() || _files[_traceFileName].lock() == NULL) {
		_traceFile = boost::shared_ptr<std::ofstream>(new std::ofstream(_traceFileName.c_str()));
		if (!_traceFile)
			return false;
		_files[_traceFileName] = _traceFile;
	} else {
		_traceFile = _files[_traceFileName].lock();
	}
	return true;
}

void Traceable::trace(const std::string& traceMsg, std::map<std::string, std::string> info) {
	if (_traceFile) {
		ScopeLock lock(_mutex);

		info["threadId"] = toStr(Thread::getThreadId());
		info["this"] = toStr(this);

		(*_traceFile) << Thread::getTimeStampMs() <<  ": " << traceMsg << " ### ";
		std::map<std::string, std::string>::iterator infoIter = info.begin();
		if (infoIter != info.end()) {
			(*_traceFile) << "[ ";
			while(infoIter != info.end()) {
				(*_traceFile) << infoIter->first << "=" << infoIter->second << " ## ";
				infoIter++;
			}
			(*_traceFile) << "]";
		}
		(*_traceFile) << std::endl;
	}
}

void Traceable::replay(const std::string& filename) {
	uint64_t now = 0;
	int64_t fileOffset = 0;
	std::ifstream traceFile(filename.c_str());
	if (!traceFile) {
		LOG_WARN("Replaying %s: Cannot open file", filename.c_str());
		return;
	}
	string line = "";
	while (!traceFile.eof()) {
		std::getline(traceFile, line);
		//std::cout << line << std::endl;
		size_t lastMatch = 0;

		// match time of trace
		size_t currMatch = line.find(": ");
		uint64_t linetime = strTo<uint64_t>(line.substr(lastMatch, currMatch));
		lastMatch = currMatch;

		// match message
		lastMatch += 2;
		currMatch = line.find(" ###", lastMatch);
		if (currMatch == string::npos)
			continue;
		string msg = line.substr(lastMatch, currMatch - lastMatch);
		lastMatch = currMatch;

		// match info
		std::map<std::string, std::string> info;
		currMatch = line.find(" [ ", lastMatch);
		if (currMatch != string::npos) {
			lastMatch = currMatch + 3;
			while(true) {
				currMatch = line.find("=", lastMatch);
				if (currMatch == string::npos)
					break;

				string key = line.substr(lastMatch, currMatch - lastMatch);
				lastMatch = currMatch + 1;
				currMatch = line.find(" ## ", lastMatch);

				string value = line.substr(lastMatch, currMatch - lastMatch);
				info[key] = value;
				lastMatch = currMatch + 4;
			}
		}

		// wait until we are in sync with the trace event
		now = Thread::getTimeStampMs();
		if (fileOffset == 0)
			fileOffset = now - linetime;

		int64_t toWait = (linetime + fileOffset) - now;
		if (toWait > 0) {
			Thread::sleepMs(toWait);
		}

//    std::cout << msg << " [ ";
//    std::map<std::string, std::string>::iterator infoIter = info.begin();
//    while(infoIter != info.end()) {
//      std::cout << infoIter->first << "=" << infoIter->second << " ";
//      infoIter++;
//    }
//    std::cout << "]" << std::endl;

		retrace(msg, info);
	}
}


/*
	std::string _traceFileName;
	boost::shared_ptr<std::ofstream> _streams;

 */


}