//
//  uPublisher.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 14/1/12.
//  Copyright (c) 2012 TU Darmstadt. All rights reserved.
//

#ifndef UMPUBLISHER_H_LELDDAOE
#define UMPUBLISHER_H_LELDDAOE

#import <Foundation/Foundation.h>
#import <umundo/core.h>

@interface UMPublisher :
NSObject {
	@public
	boost::shared_ptr<umundo::Publisher> _cppPub;
}

- (id) initWithChannel:
(NSString*) channelName;
- (void)send:
(NSData*)data;
- (void)sendMsg:
(umundo::Message*)msg;

@end

#endif /* end of include guard: UMPUBLISHER_H_LELDDAOE */
