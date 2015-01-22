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

#ifndef UMDISCOVERY_H_C9FD5865
#define UMDISCOVERY_H_C9FD5865

#import <Foundation/Foundation.h>

#include <string>
#include <boost/shared_ptr.hpp>

#include <umundo/core.h>
#include <umundo-objc/core/UMNode.h>

/**
 * Objective-C representation of Node%s.
 */
@interface UMDiscovery :
NSObject {
	SharedPtr<umundo::Discovery> _cppDiscovery;
}

- (id) init;
- (void)add:
(UMNode*)node;
- (void)remove:
(UMNode*)node;

@end

#endif /* end of include guard: UMDISCOVERY_H_C9FD5865 */
