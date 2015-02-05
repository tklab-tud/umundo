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

#ifndef RTPThread_H_XQPJWLQR
#define RTPThread_H_XQPJWLQR

#include "umundo/core/Common.h"

#include <cstring>
#include <boost/function.hpp>

#include "umundo/config.h"
#include "umundo/core/thread/Thread.h"

#include "umundo/core/connection/rtp/libre.h"

namespace umundo {

class RTPThread : public Thread {
public:
	~RTPThread();
	static SharedPtr<RTPThread> getInstance();

	int call(boost::function<int()>);

protected:

	static struct libre::mqueue *_mq;
	static RMutex _mutex;
	static Monitor _cond;
	static boost::function<int()> _func;
	static int _retval;
	static uint64_t _id;

	static void handler(int, void*, void*);

private:
	void run();

	RTPThread();
	static WeakPtr<RTPThread> _instance;
};

}

#endif /* end of include guard: RTPThread_H_XQPJWLQR */
