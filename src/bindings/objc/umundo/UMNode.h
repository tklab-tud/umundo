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

<<<<<<< HEAD:src/bindings/objc/umundo/UMNode.h
#include "umundo/UMPublisher.h"
#include "umundo/UMSubscriber.h"
#include "umundo/connection/Node.h"
=======
#include <umundo/core-objc.h>
#include <umundo/core/UMPublisher.h>
#include <umundo/core/UMSubscriber.h>
>>>>>>> f326cc1a76b2d5314e8cd9e5e17509f2f3270605:src/bindings/core/objc/umundo/core/UMNode.h

/**
 * Objective-C representation of Node%s.
 */
@interface UMNode :
NSObject {
	@public
	SharedPtr<umundo::Node> _cppNode;
}

- (id) init;
- (void)addPublisher:(UMPublisher*)pub;
- (void)addSubscriber:(UMSubscriber*)sub;
- (void)removePublisher:(UMPublisher*)pub;
- (void)removeSubscriber:(UMSubscriber*)sub;

@end

#endif /* end of include guard: UMNODE_H_9C2MCZDC */
