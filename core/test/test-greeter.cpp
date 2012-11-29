#include "umundo/core.h"
#include <iostream>

using namespace umundo;

int instances = 0;

class TestGreeter : public Greeter {
	void welcome(Publisher, const string& nodeId, const string& subId) {
    instances++;
  }
	
  void farewell(Publisher, const string& nodeId, const string& subId) {
    instances--;
  }

};

bool testGreeter() {
  Node node1(Host::getHostId());
  Node node2(Host::getHostId());
  
  Publisher pub1("foobar", new TestGreeter());
  
  node1.addPublisher(pub1);
  
  Subscriber sub1("f");
  node2.addSubscriber(sub1);
  
  pub1.waitForSubscribers(1);
  assert(instances == 1);
  
  node2.removeSubscriber(sub1);

  while(pub1.waitForSubscribers(0) != 0)
    Thread::sleepMs(20);
  
  assert(instances == 0);
  return true;
}

int main(int argc, char** argv) {
	if(!testGreeter())
		return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
