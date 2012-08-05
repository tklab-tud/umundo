#include "umundo/core.h"
#include <iostream>

using namespace umundo;

bool testRecursiveMutex() {
	Mutex mutex;
	UMUNDO_LOCK(mutex);
	if(!mutex.tryLock()) {
		LOG_ERR("tryLock should be possible from within the same thread");
		assert(false);
	}
	UMUNDO_LOCK(mutex);
	// can we unlock it as well?
	UMUNDO_UNLOCK(mutex);
	UMUNDO_UNLOCK(mutex);
	return true;
}

static Mutex testMutex;
bool testThreads() {
	class Thread1 : public Thread {
		void run() {
			if(testMutex.tryLock()) {
				LOG_ERR("tryLock should return false with a mutex locked in another thread");
				assert(false);
			}
			UMUNDO_LOCK(testMutex); // blocks
			Thread::sleepMs(50);
			UMUNDO_UNLOCK(testMutex);
			Thread::sleepMs(100);
		}
	};

	/**
	 * tl=tryLock, l=lock, u=unlock b=block, sN=sleep(N)
	 * thread1:        start tl l  l s50      u
	 *    main: start l s50       u s20  tl    l join
	 *    t->          0         50     70   100
	 */

	UMUNDO_LOCK(testMutex);
	Thread1 thread1;
	thread1.start();
	Thread::sleepMs(50); // thread1 will trylock and block on lock
	UMUNDO_UNLOCK(testMutex);  // unlock
	Thread::sleepMs(20); // yield cpu and sleep
	// thread1 sleeps with lock on mutex
	if(testMutex.tryLock()) {
		LOG_ERR("tryLock should return false with a mutex locked in another thread");
		assert(false);
	}
	UMUNDO_LOCK(testMutex);    // thread1 will unlock and sleep
	thread1.join();      // join with thread1
	if(thread1.isStarted()) {
		LOG_ERR("thread still running after join");
		assert(false);
	}
	return true;
}

static Monitor testMonitor;
static int passedMonitor = 0;
bool testMonitors() {
	struct TestThread : public Thread {
		int _ms;
		TestThread(int ms) : _ms(ms) {}
		void run() {
			testMonitor.wait();
			Thread::sleepMs(10); // avoid clash with other threads
			passedMonitor++;
		}
	};

	TestThread thread1(0);
	TestThread thread2(5);
	TestThread thread3(10);

	for (int i = 0; i < 10; i++) {
		passedMonitor = 0;

		// all will block on monitor
		thread1.start();
		thread2.start();
		thread3.start();
		Thread::sleepMs(5); // give threads a chance to run into wait
		if(passedMonitor != 0) {
			LOG_ERR("%d threads already passed the monitor", passedMonitor);
			assert(false);
		}
		UMUNDO_SIGNAL(testMonitor); // signal a single thread
		Thread::sleepMs(15); // thread will increase passedMonitor
		if(passedMonitor != 1) {
			LOG_ERR("Expected 1 threads to pass the monitor, but %d did", passedMonitor);
			assert(false);
		}
		UMUNDO_BROADCAST(testMonitor); // signal all other threads
		Thread::sleepMs(15);

		if (thread1.isStarted() || thread2.isStarted() || thread3.isStarted()) {
			LOG_ERR("Threads ran to completion but still insist on being started");
			assert(false);
		}
	}
	return true;
}

static Monitor testTimedMonitor;
static int passedTimedMonitor = 0;
bool testTimedMonitors() {
	struct TestThread : public Thread {
		int _ms;
		TestThread(int ms) : _ms(ms) {}
		void run() {
			testTimedMonitor.wait(_ms);
			passedTimedMonitor++;
		}
	};

	TestThread thread1(15);
	TestThread thread2(0); // waits forever
	TestThread thread3(0); // waits forever
	TestThread thread4(0); // waits forever
	TestThread thread5(0); // waits forever

	for (int i = 0; i < 50; i++) {
		// test waiting for a given time
		passedTimedMonitor = 0;
		thread1.start(); // wait for 15ms at mutex before resuming
		Thread::sleepMs(5);
		assert(passedTimedMonitor == 0); // thread1 should not have passed
		Thread::sleepMs(25);
		assert(passedTimedMonitor == 1); // thread1 should have passed
		assert(!thread1.isStarted());

		// test signalling a set of threads
		passedTimedMonitor = 0;
		thread2.start();
		thread3.start();
		thread4.start();
		thread5.start();
		Thread::sleepMs(5);
		testTimedMonitor.signal(2); // signal 2 threads
		Thread::sleepMs(10);
		assert(passedTimedMonitor == 2);
		testTimedMonitor.signal(1); // signal another thread
		Thread::sleepMs(5);
		assert(passedTimedMonitor == 3);
		testTimedMonitor.broadcast(); // signal last thread
		Thread::sleepMs(5);
		assert(passedTimedMonitor == 4);
		assert(!thread2.isStarted() && !thread3.isStarted() && !thread4.isStarted() && !thread5.isStarted());

		// test timed and unlimited waiting
		passedTimedMonitor = 0;
		thread1.start();
		thread2.start(); // with another thread
		thread3.start(); // with another thread
		Thread::sleepMs(5);
		testTimedMonitor.signal(); // explicit signal
		Thread::sleepMs(10);
		assert(passedTimedMonitor == 1);
		// wo do not know which thread passed
		assert(!thread1.isStarted() || !thread2.isStarted() || !thread3.isStarted());
		if (thread1.isStarted()) {
			// thread1 is still running, just wait
			Thread::sleepMs(10);
			assert(passedTimedMonitor == 2);
		}
		testTimedMonitor.broadcast(); // explicit signal
		Thread::sleepMs(5);
		assert(passedTimedMonitor == 3);
		assert(!thread1.isStarted() && !thread2.isStarted() && !thread3.isStarted());

		// test signalling prior to waiting
		passedTimedMonitor = 0;
		testTimedMonitor.signal();
		thread1.start();
		Thread::sleepMs(5);
		thread2.start();
		Thread::sleepMs(5);
		assert(passedTimedMonitor == 1);
		assert(!thread1.isStarted());
		assert(thread2.isStarted());
		testTimedMonitor.signal();
		Thread::sleepMs(5);
		assert(passedTimedMonitor == 2);

		assert(!thread1.isStarted());
		assert(!thread2.isStarted());
		assert(!thread3.isStarted());

	}
	return true;
}

class FooTracer : public Traceable, public Thread {
	void run() {
		while(isStarted()) {
			trace("This is foo");
			Thread::sleepMs(20);
		}
	}
};

bool testTracing() {
	FooTracer* tr1 = new FooTracer();
	FooTracer* tr2 = new FooTracer();
	FooTracer* tr3 = new FooTracer();

	tr1->setTraceFile("trace.txt");
	tr2->setTraceFile("trace.txt");
	tr3->setTraceFile("trace.txt");

	tr1->start();
	tr2->start();
	tr3->start();

	Thread::sleepMs(100);
	delete tr1;
	delete tr2;
	delete tr3;

	return true;
}

int main(int argc, char** argv) {
	if(!testTracing())
		return EXIT_FAILURE;
	if(!testRecursiveMutex())
		return EXIT_FAILURE;
	if(!testThreads())
		return EXIT_FAILURE;
	if(!testMonitors())
		return EXIT_FAILURE;
	if(!testTimedMonitors())
		return EXIT_FAILURE;
}
