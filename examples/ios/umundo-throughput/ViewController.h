//
//  ViewController.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

@interface ViewController : UIViewController<UMReceiver>
@property (retain, nonatomic) IBOutlet UITextView *text;
@property (retain, nonatomic) NSLock* lock;
@property (retain, nonatomic) NSTimer* timer;
@property (retain, nonatomic) UMSubscriber* rtpSub;
@property (retain, nonatomic) UMSubscriber* mcastSub;
@property (retain, nonatomic) UMSubscriber* tcpSub;
@property (retain, nonatomic) UMPublisher* reporter;

@property (nonatomic) NSUInteger bytesRcvd;
@property (nonatomic) NSUInteger pktsDropped;
@property (nonatomic) NSUInteger pktsRecvd;
@property (nonatomic) UInt64 lastSeqNr;
@property (nonatomic) UInt64 lastTimeStamp;
@property (nonatomic) UInt64 currTimeStamp;

- (void)accumulateStats;
- (void)addText:(NSString*)theText;
- (void)clearText;


@end
