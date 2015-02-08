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

#include "umundo/config.h"
#include "umundo/core/Debug.h"
#include "umundo/core/thread/Thread.h"

#include <string.h> // strerror

#include <sys/types.h>

#if defined(UNIX) || defined(IOS)
#include <sys/time.h> // gettimeofday
#endif

namespace umundo {

Thread::Thread() {
	_isStarted = false;
	_thread = NULL;
}

Thread::~Thread() {
	if (_thread) {
		if (_isStarted) {
			stop();
			join();
		}
		delete _thread;
	}
}

void Thread::join() {
	if (_thread)
		_thread->join();
	_isStarted = false;
}

#ifdef WITH_CPP11
static unsigned long int _threadIDToInt(const std::thread::id &aHandle) {
	static std::mutex idMapLock;
	static std::map<std::thread::id, unsigned long int> idMap;
	static unsigned long int idCount(1);

	std::lock_guard<std::mutex> guard(idMapLock);
	if(idMap.find(aHandle) == idMap.end())
		idMap[aHandle] = idCount ++;
	return idMap[aHandle];
}
#endif

unsigned long int Thread::getThreadId() {
	std::stringstream ssThreadId;
	
#ifdef WITH_CPP11
	ssThreadId << _threadIDToInt(tthread::this_thread::get_id());
#else
	ssThreadId << tthread::this_thread::get_id();
#endif
	int threadId;
	ssThreadId >> threadId;
	return threadId;
}

void Thread::start() {
	if (_isStarted)
		return;
	_isStarted = true;
	_thread = new tthread::thread(runWrapper, this);
}

void Thread::stop() {
	if (_isStarted)
		_isStarted = false;
}

void Thread::runWrapper(void *obj) {
	Thread* t = (Thread*)obj;
	t->run();
	t->_isStarted = false;
}

void Thread::yield() {
	tthread::this_thread::yield();
}

void Thread::sleepMs(uint32_t ms) {
	tthread::this_thread::sleep_for(tthread::chrono::milliseconds(ms));
}

// active wait on windows for sub milli-second values
void Thread::sleepUs(uint32_t us) {
#ifdef WIN32
	__int64 time1 = 0, time2 = 0, freq = 1000000;

	QueryPerformanceCounter((LARGE_INTEGER *) &time1);
	QueryPerformanceFrequency((LARGE_INTEGER *) &freq);

	do {
		QueryPerformanceCounter((LARGE_INTEGER *) &time2);
	} while((time2-time1) < us * 10);
#else
	tthread::this_thread::sleep_for(tthread::chrono::microseconds(us));
#endif
}

uint64_t Thread::getTimeStampMs() {
	uint64_t time = 0;
#ifdef WIN32
	FILETIME tv;
	GetSystemTimeAsFileTime(&tv);
	time = (((uint64_t) tv.dwHighDateTime) << 32) + tv.dwLowDateTime;
	time /= 10000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time += tv.tv_sec;
	time *= 1000LU;
	time += tv.tv_usec / 1000;
#endif
	return time;
}

//Monitor::Monitor(const Monitor& other) {
//	UM_LOG_ERR("CopyConstructor!");
//}

Monitor::Monitor() {
}

Monitor::~Monitor() {
}

void Monitor::signal() {
	_cond.notify_one();
}

void Monitor::broadcast() {
	_cond.notify_all();
}

void Monitor::signal(int nrThreads) {
	while(nrThreads--)
		_cond.notify_one();
}

void Monitor::wait(RMutex& mutex, uint32_t ms) {
	if (ms <= 0) {
		_cond.wait(mutex);
	} else {
#ifdef WITH_CPP11
		_cond.wait_for(mutex, std::chrono::milliseconds(ms));
#else
		_cond.wait_for(mutex, ms);
#endif
	}
}

#ifdef THREAD_PTHREAD
#endif
#ifdef THREAD_WIN32
#endif

}