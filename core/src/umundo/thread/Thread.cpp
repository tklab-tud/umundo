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
#include "umundo/common/Debug.h"
#include "umundo/thread/Thread.h"

#include <string.h> // strerror

#include <sys/types.h>

#if defined(UNIX) || defined(IOS)
#include <sys/time.h> // gettimeofday
#endif

namespace umundo {

Thread::Thread() {
	_isStarted = false;
//	_thread = NULL; // pthread_t is actually typedef'd to "long unsigned int"
}

Thread::~Thread() {
	if (_isStarted) {
		stop();
		join();
	}

#ifdef THREAD_PTHREAD
	if (_thread)
		pthread_detach(_thread);
#endif
#ifdef THREAD_WIN32
	if (_thread != NULL)
		CloseHandle(_thread);
#endif
}

void Thread::join() {
#ifdef THREAD_PTHREAD
	int err = 0;
	err = pthread_join(_thread, NULL);
	switch (err) {
	case EDEADLK:
		LOG_ERR("join: deadlock detected");
		break;
	case EINVAL:
		LOG_ERR("join: trying to join unjoinable thread");
		break;
	case ESRCH:
		LOG_ERR("join: no such thread");
		break;
	default:
//		_thread = NULL;
		break;
	}
#endif
#ifdef THREAD_WIN32
	DWORD dwCode;
	dwCode = WaitForSingleObject(_thread, INFINITE);
	assert(dwCode==WAIT_OBJECT_0);
	GetExitCodeThread(_thread, &dwCode);
	assert(dwCode != STILL_ACTIVE);
#endif

}

int Thread::getThreadId() {
	static int _nextThreadId = 1;
	static Mutex _threadIdMutex;
#ifdef THREAD_PTHREAD
	static std::map<pthread_t, int> ids;
	pthread_t pt = pthread_self();
#endif
#ifdef THREAD_WIN32
	static std::map<DWORD, int> ids;
	DWORD pt = GetCurrentThreadId();
#endif
	// locking breaks sometimes at pthread_mutex_lock
	_threadIdMutex.lock();
	if (ids.find(pt) == ids.end()) {
		ids[pt] = _nextThreadId++;
	}
	_threadIdMutex.unlock();
	return ids[pt];
}

void Thread::start() {
	if (_isStarted)
		return;
	_isStarted = true;

#ifdef THREAD_PTHREAD
	/*
		if (_thread)
			join();

	 */

	int err = pthread_create(&_thread, NULL, &runWrapper, (void*)this);
	if (err != 0) {
		switch (err) {
		case EAGAIN:
			LOG_WARN("pthread_create failed: Insufficient resources to create another thread.");
			break;
		case EINVAL:
			LOG_WARN("pthread_create failed: Invalid settings in attr.");
			break;
		case EPERM:
			LOG_WARN("pthread_create failed: No permissions to set scheduling policy.");
			break;
		}
	}
#endif
#ifdef THREAD_WIN32
	DWORD threadId;
	_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)runWrapper, this, 0, &threadId);
	assert(_thread != NULL);
#endif
}

#ifdef THREAD_PTHREAD
void* Thread::runWrapper(void *obj) {
	Thread* t = (Thread*)obj;
	t->run();
	// @TODO: Is there a race-condition here?
	t->_isStarted = false;
	pthread_exit(NULL);
	return NULL;
}
#endif
#ifdef THREAD_WIN32
DWORD Thread::runWrapper(Thread *t) {
	t->run();
	t->_isStarted = false;
	return 0;
}
#endif

void Thread::stop() {
	_isStarted = false;
}

void Thread::yield() {
#ifdef THREAD_PTHREAD
	int err = sched_yield();
	(void)err;
	assert(!err);
#endif
#ifdef THREAD_WIN32
	SwitchToThread();
#endif
}

void Thread::sleepMs(uint32_t ms) {
#ifdef THREAD_PTHREAD
// System Calls may return early when debugging with gdb!
// http://sourceware.org/gdb/onlinedocs/gdb/Interrupted-System-Calls.html
	int rv;
	do {
		rv = usleep(1000 * ms);
	} while(rv == -1 && errno == EINTR);
#endif
#ifdef THREAD_WIN32
	Sleep(ms);
#endif
}

uint64_t Thread::getTimeStampMs() {
	uint64_t time = 0;
#ifdef WIN32
	FILETIME tv;
	GetSystemTimeAsFileTime(&tv);
	time = (((uint64_t) tv.dwHighDateTime) << 32) + tv.dwLowDateTime;
	time /= 10000;
#endif
#ifdef UNIX
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time += tv.tv_sec * 1000;
	time += tv.tv_usec / 1000;
#endif
	return time;
}

Mutex::Mutex() {
#ifdef THREAD_PTHREAD
	pthread_mutexattr_t attrib;
	int ret = pthread_mutexattr_init(&attrib);
	if (ret != 0)
		switch(ret) {
		case ENOMEM:
			LOG_ERR("pthread_mutexattr_init: %s", strerror(ret));
			break;
		default:
			LOG_ERR("pthread_mutexattr_init: unknown error");
			break;
		}
	assert(ret == 0);

	ret = pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE);
	if (ret != 0)
		switch(ret) {
		case EINVAL:
			LOG_ERR("pthread_mutexattr_settype: %s", strerror(ret));
			break;
		default:
			LOG_ERR("pthread_mutexattr_settype: unknown error");
			break;
		}
	assert(ret == 0);

	pthread_mutex_init(&_mutex, &attrib);
	pthread_mutexattr_destroy(&attrib);
#endif
#ifdef THREAD_WIN32
	_mutex = CreateMutex(NULL, FALSE, NULL);
	if (_mutex == NULL) {
		LOG_ERR("CreateMutex: %s", GetLastError());
	}
	assert(_mutex != NULL);
#endif
}

Mutex::~Mutex() {
#ifdef THREAD_PTHREAD
	pthread_mutex_destroy(&_mutex);
#endif
#ifdef THREAD_WIN32
	CloseHandle(_mutex);
#endif
}

void Mutex::lock() {
#ifdef THREAD_PTHREAD
	int err = pthread_mutex_lock(&_mutex);
	if (err)
		LOG_ERR("pthread_mutex_lock: %s", strerror(err));
	(void)err;
#endif
#ifdef THREAD_WIN32
	WaitForSingleObject(_mutex, INFINITE);
#endif
}

bool Mutex::tryLock() {
#ifdef THREAD_PTHREAD
	int rv = pthread_mutex_trylock(&_mutex);
	if(rv == EBUSY)
		return false;
#endif
#ifdef THREAD_WIN32
	if(WaitForSingleObject(_mutex, 0) == WAIT_TIMEOUT)
		return false;
#endif
	return true;
}

void Mutex::unlock() {
#ifdef THREAD_PTHREAD
	pthread_mutex_unlock(&_mutex);
#endif
#ifdef THREAD_WIN32
	ReleaseMutex(_mutex);
#endif

}

ScopeLock::ScopeLock(Mutex* mutex) : _mutex(mutex) {
	_mutex->lock();
}

ScopeLock::~ScopeLock() {
	_mutex->unlock();
}

//Monitor::Monitor(const Monitor& other) {
//	LOG_ERR("CopyConstructor!");
//}

Monitor::Monitor() {
	_waiters = 0;
	_signaled = 0;
#ifdef THREAD_PTHREAD
	int err;
	err = pthread_mutex_init(&_mutex, NULL);
	assert(err == 0);
	err = pthread_cond_init(&_cond, NULL);
	assert(err == 0);
	(void)err; // avoid unused warning
#endif
#ifdef THREAD_WIN32
	// auto resetting event
	_monitor = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
}

Monitor::~Monitor() {
	broadcast();
#ifdef THREAD_PTHREAD
	int err;
	while((err = pthread_cond_destroy(&_cond))) {
		switch(err) {
		case EBUSY:
			LOG_WARN("Trying to destroy locked monitor - retrying");
			break;
		default:
			LOG_ERR("pthread_mutex_destroy returned %d");
			goto failed_cond_destroy;
		}
		Thread::sleepMs(50);
	}
failed_cond_destroy:
	while((err = pthread_mutex_destroy(&_mutex))) {
		switch(err) {
		case EBUSY:
			LOG_WARN("Trying to destroy locked monitor - retrying");
			break;
		default:
			LOG_ERR("pthread_mutex_destroy returned %d");
			goto failed_mutex_destroy;
		}
		Thread::sleepMs(50);
	}
failed_mutex_destroy:
	return;
#endif
#ifdef THREAD_WIN32
	CloseHandle(_monitor);
#endif
}

void Monitor::signal() {
	signal(1);
}

void Monitor::broadcast() {
	signal(_waiters);
}

void Monitor::signal(int nrThreads) {
	// if we are signalled prior to waiting, allow the first thread to pass
	if (_waiters > 0) {
		_signaled = (nrThreads > _waiters ? _waiters : nrThreads);
	} else {
		_signaled = 1;
	}
//	LOG_ERR("W:%d - N:%d - S:%d", _waiters, nrThreads, _signaled);
#ifdef THREAD_PTHREAD
	pthread_mutex_lock(&_mutex);
	pthread_cond_broadcast(&_cond);
	pthread_mutex_unlock(&_mutex);
#endif
#ifdef THREAD_WIN32
	_monitorLock.lock();
	while(_signaled > 0) {
		SetEvent(_monitor);
		_signaled--;
	}
	_monitorLock.unlock();
#endif
	assert(_waiters >= 0);
	assert(_signaled >= 0);
}

bool Monitor::wait(uint32_t ms) {
#ifdef THREAD_PTHREAD
	int rv = 0;
	pthread_mutex_lock(&_mutex);
//	LOG_ERR("W:%d - S:%d", _waiters, _signaled);

	// signaled prior to waiting
	if (_signaled > 0) {
		_signaled--;
		pthread_mutex_unlock(&_mutex);
		return true;
	}

	_waiters++;
	// wait indefinitely
	if (ms == 0) {
		while(!_signaled) // are there enough signals for this thread to pass?
			rv = pthread_cond_wait(&_cond, &_mutex);
		assert(_signaled > 0);
		assert(_waiters > 0);
		_signaled--;
		_waiters--;
		pthread_mutex_unlock(&_mutex);
		return rv == 0;
	} else {
		// get endtime for waiting
		struct timeval tv;
		gettimeofday(&tv, NULL);
		assert(tv.tv_usec < 1000000);
//		tv.tv_usec += (ms % 1000) * 1000;
//		tv.tv_sec += (ms / 1000) + (tv.tv_usec / 1000000);
//		tv.tv_usec %= 1000000;
		struct timespec ts;
		ts.tv_sec = tv.tv_sec + (ms / 1000);
		ts.tv_nsec = (tv.tv_usec * 1000); // convert tv microseconds to nanoseconds
		ts.tv_nsec += (ms % 1000) * 1000000; // add millisecond part of wait time

		// were we signaled or timed out?
		while(!_signaled && rv != ETIMEDOUT) {
			rv = pthread_cond_timedwait(&_cond, &_mutex, &ts);
//      if (!rv)
//        break;
//      LOG_ERR("%d %s", rv, strerror(rv));
		}
		// decrease number of signals if we awoke due to signal
		if (rv != ETIMEDOUT) {
			assert(_signaled > 0);
			_signaled--;
		}
		assert(_waiters > 0);
		_waiters--;
		pthread_mutex_unlock(&_mutex);
		return rv == 0;
	}
#endif
#ifdef THREAD_WIN32
	_monitorLock.lock();

	// signaled prior to waiting
	if (_signaled > 0) {
		_signaled--;
		_monitorLock.unlock();
		return true;
	}

	_waiters++;
	if (ms == 0)
		ms = INFINITE;
	_monitorLock.unlock();
	int result = WaitForSingleObject(_monitor, ms);
	_monitorLock.lock();
	_waiters--;
	_monitorLock.unlock();

	return result == WAIT_OBJECT_0;
#endif
	assert(_waiters >= 0);
	assert(_signaled >= 0);
}

#ifdef THREAD_PTHREAD
#endif
#ifdef THREAD_WIN32
#endif

}