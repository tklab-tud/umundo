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

#ifndef RTPHELPERS_H_XQPJWLQR
#define RTPHELPERS_H_XQPJWLQR

#include <cstring>
#include <boost/function.hpp>

#include "umundo/config.h"
#include "umundo/common/Common.h"
#include "umundo/thread/Thread.h"

#include "umundo/connection/rtp/libre.h"

namespace umundo {

class RTPHelpers : public Thread {
protected:
	RTPHelpers();
	~RTPHelpers();

	static uint32_t _libreUsage;
	static bool _initDone;
	static struct libre::mqueue *_mq;
	static RMutex _mutex;
	static RMutex _usageCountMutex;
	static Monitor _cond;
	static boost::function<int()> _func;
	static int _retval;
	static uint64_t _id;

	static int call(boost::function<int()>);
	static void handler(int, void*, void*);

private:
	void run();

	friend class RTPSubscriber;
	friend class RTPPublisher;
};

}

#endif /* end of include guard: RTPHELPERS_H_XQPJWLQR */
