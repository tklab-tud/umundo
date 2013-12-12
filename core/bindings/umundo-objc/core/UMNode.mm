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

#import "UMNode.h"

@implementation UMNode

- (id) init {
  self = [super init];
  if(self) {
		_cppNode = boost::shared_ptr<umundo::Node>(new umundo::Node());
  }
  return self;
}

- (void)addPublisher:(UMPublisher*)pub {
  _cppNode->addPublisher(*(pub->_cppPub.get()));
}

- (void)addSubscriber:(UMSubscriber*)sub {
  _cppNode->addSubscriber(*(sub->_cppSub.get()));
}

@end
