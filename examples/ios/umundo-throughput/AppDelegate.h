//
//  AppDelegate.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <umundo/s11n-objc.h>
#import <umundo/core-objc.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate, UMReceiver>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) UMNode *node;
@property (strong, nonatomic) UMDiscovery *disc;
@property (retain, nonatomic) UMSubscriber* rtpSub;
@property (retain, nonatomic) UMSubscriber* mcastSub;
@property (retain, nonatomic) UMSubscriber* tcpSub;
@property (retain, nonatomic) UMPublisher* reporter;

@property (retain, nonatomic) NSLock* lock;
@property (retain, nonatomic) NSTimer* timer;

@property (nonatomic) NSUInteger bytesRcvd;
@property (nonatomic) NSUInteger pktsDropped;
@property (nonatomic) NSUInteger pktsRecvd;
@property (nonatomic) UInt64 lastSeqNr;
@property (nonatomic) UInt64 lastTimeStamp;
@property (nonatomic) UInt64 currTimeStamp;
@property (nonatomic) UInt64 startedTimeStamp;
@property (nonatomic) UInt64 firstTimeStamp;
@property (nonatomic) UInt32 reportInterval;

@end
