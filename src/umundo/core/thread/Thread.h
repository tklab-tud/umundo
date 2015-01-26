/**
 *  @file
 *  @brief      Threading and concurrency primitives.
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

#ifndef PTHREAD_H_KU2YWI3W
#define PTHREAD_H_KU2YWI3W
#include "umundo/core/Common.h"

#ifndef WITH_CPP11
#include "umundo/core/thread/tinythread.h"
#endif

// C++11
#ifdef WITH_CPP11
#include <mutex>
#include <thread>
#include <condition_variable>
#endif

#ifdef WITH_CPP11
#define tthread std
#endif

// this is a hack until we get a compiler firewall per Pimpl
#ifndef WITH_CPP11
#	ifdef _WIN32
#		if !(defined THREAD_PTHREAD || defined THREAD_WIN32)
#			define THREAD_WIN32 1
#		endif
#	else
#		if !(defined THREAD_PTHREAD || defined THREAD_WIN32)
#			define THREAD_PTHREAD 1
#		endif
#	endif

#	if !(defined THREAD_PTHREAD || defined THREAD_WIN32)
#		error No thread implementation choosen
#	endif

#	ifdef THREAD_PTHREAD
#		include <pthread.h>
#		include <errno.h>
#	endif
#	ifdef THREAD_WIN32
#		define WIN32_LEAN_AND_MEAN
#		define NOMINMAX
#		include <windows.h>
#		undef WIN32_LEAN_AND_MEAN
#	endif
#endif


#ifdef DEBUG_THREADS
#define UMUNDO_SCOPE_LOCK(mutex) \
UM_LOG_DEBUG("Locking mutex %p", &mutex); \
RScopeLock lock(mutex);

#define UMUNDO_LOCK(mutex) \
UM_LOG_DEBUG("Locking mutex %p", &mutex); \
mutex.lock(); \
UM_LOG_DEBUG("Locked mutex %p", &mutex);

#define UMUNDO_TRYLOCK(mutex) \
UM_LOG_DEBUG("Tying to lock mutex %p", &mutex); \
mutex.trylock();

#define UMUNDO_UNLOCK(mutex) \
UM_LOG_DEBUG("Unlocking mutex %p", &mutex); \
mutex.unlock();

#define UMUNDO_WAIT(monitor) \
UM_LOG_DEBUG("Waiting at monitor %p", &monitor); \
monitor.wait(); \
UM_LOG_DEBUG("Signaled at monitor %p", &monitor);

#define UMUNDO_WAIT(monitor, ms) \
UM_LOG_DEBUG("Waiting for %dms at monitor %p", ms, &monitor); \
monitor.wait(ms); \
UM_LOG_DEBUG("Awaken at monitor %p", &monitor);

#define UMUNDO_SIGNAL(monitor) \
UM_LOG_DEBUG("Signaling monitor %p", &monitor); \
monitor.signal();

#define UMUNDO_BROADCAST(monitor) \
UM_LOG_DEBUG("Signaling all monitor %p", &monitor); \
monitor.broadcast();
#endif

#ifndef DEBUG_THREADS
#define UMUNDO_SCOPE_LOCK(mutex) RScopeLock lock(mutex);
#define UMUNDO_LOCK(mutex) mutex.lock();
#define UMUNDO_TRYLOCK(mutex) mutex.tryLock();
#define UMUNDO_UNLOCK(mutex) mutex.unlock();
#define UMUNDO_WAIT(monitor, mutex) monitor.wait(mutex);
#define UMUNDO_WAIT2(monitor, mutex, timeout) monitor.wait(mutex, timeout);
#define UMUNDO_SIGNAL(monitor) monitor.signal();
#define UMUNDO_BROADCAST(monitor) monitor.broadcast();
#endif

namespace umundo {

/**
 * Platform independent parallel control-flows.
 */
class UMUNDO_API Thread {
public:
	Thread();
	virtual ~Thread();
	virtual void run() = 0;
	virtual void join();
	void start();
	void stop();
	bool isStarted() {
		return _isStarted;
	}

	bool isJoinable() {
		if (_thread)
			return _thread->joinable();
		return false;
	}

	static void yield();
	static void sleepMs(uint32_t ms);
	static void sleepUs(uint32_t us);
	static unsigned long int getThreadId(); ///< integer unique to the current thread
	static uint64_t getTimeStampMs(); ///< timestamp in ms since 01.01.1970

private:
	bool _isStarted;
	static void runWrapper(void*);
	tthread::thread* _thread;

};

/**
 * Platform independent mutual exclusion.
 */
typedef tthread::recursive_mutex RMutex;
typedef tthread::mutex Mutex;

/**
 * Instantiate on stack to give code in scope below exclusive access.
 */
typedef tthread::lock_guard<tthread::recursive_mutex> RScopeLock;
typedef tthread::lock_guard<tthread::mutex> ScopeLock;

/**
 * See comments from Schmidt on condition variables in windows:
 * http://www.cs.wustl.edu/~schmidt/win32-cv-1.html (we choose 3.2)
 */

class UMUNDO_API Monitor {
public:
	Monitor();
//	Monitor::Monitor(const Monitor& other);
	virtual ~Monitor();

	void signal();
	void signal(int nrThreads);
	void broadcast();
	void wait(RMutex& mutex) {
		return wait(mutex, 0);
	}
	void wait(RMutex& mutex, uint32_t ms);

private:
#ifdef WITH_CPP11
	std::condition_variable_any _cond;
#else
	tthread::condition_variable _cond;
#endif
};

}

#endif /* end of include guard: PTHREAD_H_KU2YWI3W */
