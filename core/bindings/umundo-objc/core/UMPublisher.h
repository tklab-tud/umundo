/**
 *  @file
 *  @brief      Publisher implementation for Objective-C
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

#ifndef UMPUBLISHER_H_LELDDAOE
#define UMPUBLISHER_H_LELDDAOE

#import <Foundation/Foundation.h>
#import <umundo/core.h>

/**
 * Objective-C representation of a Publisher%s.
 */
@interface UMPublisher :
NSObject {
	@public
	SharedPtr<umundo::Publisher> _cppPub;
}

- (id) initWithChannel:
(NSString*) channelName;
- (void)send:
(NSData*)data;
- (void)sendMsg:
(umundo::Message*)msg;

@end

#endif /* end of include guard: UMPUBLISHER_H_LELDDAOE */
