/**
 *  @file
 *  @brief      Objective-C wrapper for typed publishers
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

#ifndef TYPEDPUBLISHER_H_9RDF6TXT
#define TYPEDPUBLISHER_H_9RDF6TXT

#import <Foundation/Foundation.h>

#include <google/protobuf/message_lite.h>
#import <umundo/core.h>
#import <umundo-objc/core.h>
#import <umundo-objc/s11n.h>

/**
 * Objective-C wrapper for object sending.
 */
@interface UMTypedPublisher :
UMPublisher {
	@public
}

- (id) initWithChannel:
(NSString*) channelName;
- (void)sendObj:
(google::protobuf::MessageLite*)data asType:
(NSString*)type;
- (void)registerType:
(NSString*)type withSerializer:
(google::protobuf::MessageLite*)serializer;

@end

#endif /* end of include guard: TYPEDPUBLISHER_H_9RTI6TXT */
