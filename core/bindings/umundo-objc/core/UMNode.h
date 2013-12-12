/**
 *  @file
 *  @brief      Node implementation for Objective-C
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

#ifndef UMNODE_H_9C2MCZDC
#define UMNODE_H_9C2MCZDC

#import <Foundation/Foundation.h>

#include <string>
#include <boost/shared_ptr.hpp>

#include <umundo/core.h>
#include <umundo-objc/core/UMPublisher.h>
#include <umundo-objc/core/UMSubscriber.h>

/**
 * Objective-C representation of Node%s.
 */
@interface UMNode :
NSObject {
	boost::shared_ptr<umundo::Node> _cppNode;
}

- (id) init;
- (void)addPublisher:
(UMPublisher*)pub;
- (void)addSubscriber:
(UMSubscriber*)sub;

@end

#endif /* end of include guard: UMNODE_H_9C2MCZDC */
