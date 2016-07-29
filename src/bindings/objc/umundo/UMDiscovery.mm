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

#ifndef __has_extension
#define __has_extension __has_feature
#endif

#import "UMDiscovery.h"
#include "umundo/connection/Node.h"

@implementation UMDiscovery

- (id) init {
  self = [super init];
  if(self) {
		_cppDiscovery = SharedPtr<umundo::Discovery>(new umundo::Discovery(umundo::Discovery::MDNS));
  }
  return self;
}

- (void)add:(UMNode*)node {
  _cppDiscovery->add(*(node->_cppNode.get()));
}

- (void)remove:(UMNode*)node {
  _cppDiscovery->remove(*(node->_cppNode.get()));
}

@end
