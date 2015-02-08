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
@property (nonatomic) NSUInteger pktsRecvd;
@property (nonatomic) NSUInteger pktsDropped;
@property (nonatomic) UInt64 lastSeqNr;
@property (nonatomic) UInt64 timeStampServerLast;
@property (nonatomic) UInt64 timeStampServerFirst;
@property (nonatomic) SInt32 currReportNr;
@property (nonatomic) UInt64 timeStampStartedAt;
@property (nonatomic) NSString* serverUUID;

@end
