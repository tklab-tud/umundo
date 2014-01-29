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

#include <boost/bind.hpp>
#include "umundo/connection/rtp/RTPHelpers.h"

namespace umundo {

uint32_t RTPHelpers::_libreUsage=0;
bool RTPHelpers::_initDone=false;
struct libre::mqueue *RTPHelpers::_mq=NULL;
boost::function<int()> RTPHelpers::_func=NULL;
int RTPHelpers::_retval=0;
Mutex RTPHelpers::_usageCountMutex;
Mutex RTPHelpers::_mutex;
Monitor RTPHelpers::_cond;
unsigned long RTPHelpers::_id=0;

RTPHelpers::RTPHelpers() {
	ScopeLock lock(_usageCountMutex);
	if(!_libreUsage) {
		//init libre
		int err;
		libre::rand_init();
		if((err=libre::libre_init())) {
			UM_LOG_ERR("libre init failed with error code %d", err);
			return;
		}
		//start libre mainloop and init our libre calling capabilities
		{
			ScopeLock lock(_mutex);
			if(!_initDone) {
				_initDone=true;
				libre::mqueue_alloc(&_mq, handler, NULL);
			}
		}
		start();
	}
	_libreUsage++;
}

RTPHelpers::~RTPHelpers() {
	ScopeLock lock(_usageCountMutex);
	_libreUsage--;
	if(!_libreUsage) {
		//call() needs a function object returning an int
		//--> cast void function to int function (and ignore fake "return value")
		call(boost::bind((int(*)(void))libre::re_cancel));
		join();
		libre::libre_close();
	}

	//check for memory leaks
	libre::tmr_debug();
	libre::mem_debug();
}

void RTPHelpers::run() {
	int err;
	_id=Thread::getThreadId();
	err=libre::re_main(NULL);
	{
		ScopeLock lock(_mutex);
		libre::mem_deref(_mq);
		_initDone=false;
	}
	UM_LOG_INFO("re_main() STOPPED...");
	if(err)
		UM_LOG_ERR("re_main() finished with error code %d", err);
	return;
}

/** call() wakes up the poll() or select() or epoll() in the main thread for re_main() mainloop
 * and executes the given function object in the context of the main thread
 * Use this for all libre calls where new fds are created
**/
int RTPHelpers::call(boost::function<int()> f) {
	if(_id == Thread::getThreadId()) {
		//we're already running in the mainloop thread, call supplied function directly
		return f();
	} else {
		//we're not running in the mainloop thread, instruct libre to call supplied function via our handler
		ScopeLock lock(_mutex);
		_func=f;
		libre::mqueue_push(_mq, 0, NULL);
		_cond.wait(_mutex);
		return _retval;
	}
	return 0;
}

void RTPHelpers::handler(int id, void *data, void *arg) {
	ScopeLock lock(_mutex);
	_retval=_func();
	_cond.broadcast();
}
}