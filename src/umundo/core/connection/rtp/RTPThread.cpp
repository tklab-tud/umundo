/**
 *  @file
 *  @author     2013 Thilo Molitor (thilo@eightysoft.de)
 *  @author     2013 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
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

#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif // WIN32

#include "umundo/core/connection/rtp/RTPThread.h"
#include <boost/bind.hpp>

namespace umundo {

int      RTPThread::_retval = 0;
uint64_t RTPThread::_id = 0;
struct   libre::mqueue *RTPThread::_mq = NULL;

boost::function<int()> RTPThread::_func = NULL;

RMutex RTPThread::_mutex;
Monitor RTPThread::_cond;
WeakPtr<RTPThread> RTPThread::_instance;

SharedPtr<RTPThread> RTPThread::getInstance() {
	RScopeLock lock(_mutex);
	SharedPtr<RTPThread> instance = _instance.lock();

	if (!instance) {
		instance = SharedPtr<RTPThread>(new RTPThread());
		_instance = instance;
		return instance;
	}
	return instance;
}

RTPThread::RTPThread() {
	//init libre
	int err;
	libre::rand_init();
	err = libre::libre_init();
	if (err) {
		UM_LOG_ERR("libre init failed with error code %d", err);
		return;
	}

	//start libre mainloop and init our libre calling capabilities
	libre::mqueue_alloc(&_mq, handler, NULL);
	start();
}

RTPThread::~RTPThread() {
	call(boost::bind((int(*)(void))libre::re_cancel));
	join();
	libre::libre_close();

	//check for memory leaks
	libre::tmr_debug();
	libre::mem_debug();
}

void RTPThread::run() {
	int err;
	_id = Thread::getThreadId();
	err = libre::re_main(NULL);
	{
		RScopeLock lock(_mutex);
		libre::mem_deref(_mq);
	}
	UM_LOG_INFO("re_main() STOPPED...");
	if (err) {
		UM_LOG_ERR("re_main() finished with error code %d", err);
	}
	return;
}

/** call() wakes up the poll() or select() or epoll() in the main thread for re_main() mainloop
 * and executes the given function object in the context of the main thread
 * Use this for all libre calls where new fds are created
**/
int RTPThread::call(boost::function<int()> f) {
	//we're not running in the mainloop thread, instruct libre to call supplied function via our handler
	RScopeLock lock(_mutex);
	_func = f;
	libre::mqueue_push(_mq, 0, NULL);
	_cond.wait(_mutex);
	return _retval;
}

void RTPThread::handler(int id, void *data, void *arg) {
	RScopeLock lock(_mutex);
	_retval = _func();
	_cond.broadcast();
}

}