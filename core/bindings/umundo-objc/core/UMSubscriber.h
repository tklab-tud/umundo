/**
 *  @file
 *  @brief      Subscriber implementation for Objective-C
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

#ifdef __has_feature
# if __has_feature(objc_arc)
#   define(HAS_AUTORELEASE_POOL)
# endif
#endif

#ifndef UMSUBSCRIBER_H_L4CPAZMF
#define UMSUBSCRIBER_H_L4CPAZMF

#import <Foundation/Foundation.h>
#import <umundo/core.h>

/**
 * Objective-C protocol to represent Receivers for subscribers.
 */
@protocol UMReceiver
- (void)received:(NSData*)data withMeta:(NSDictionary*)meta;
@end


/**
 * Objective-C representation of a Subscriber%s.
 */
@interface UMSubscriber :
NSObject {
	@public
	SharedPtr<umundo::Subscriber> _cppSub;
	id<UMReceiver> _receiver;
}
- (id) initTCP:(NSString*)name
			receiver:(id<UMReceiver>)receiver;

- (id) initMCast:(NSString*)name
				receiver:(id<UMReceiver>)receiver
				 mcastIP:(NSString*)ip
				portBase:(short)portBase;

- (id) initRTP:(NSString*)name
			receiver:(id<UMReceiver>)receiver;

@end

#endif /* end of include guard: UMSUBSCRIBER_H_L4CPAZMF */
