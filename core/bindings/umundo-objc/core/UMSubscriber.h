//
//  uSubscriber.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 14/1/12.
//  Copyright (c) 2012 TU Darmstadt. All rights reserved.
//

#ifndef UMSUBSCRIBER_H_L4CPAZMF
#define UMSUBSCRIBER_H_L4CPAZMF

#import <Foundation/Foundation.h>
#import <umundo/core.h>

@protocol UMSubscriberReceiver
- (void)received:
(NSData*)data withMeta:
(NSDictionary*)meta;
@end

@interface UMSubscriber :
NSObject {
	@public
	boost::shared_ptr<umundo::Subscriber> _cppSub;
	id<UMSubscriberReceiver> _receiver;
}
- (id) initWithChannel:
(NSString*)name andReceiver:
(id<UMSubscriberReceiver>)receiver;
@end

#endif /* end of include guard: UMSUBSCRIBER_H_L4CPAZMF */
