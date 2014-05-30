//
//  ViewController.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

@interface ViewController : UIViewController<UMSubscriberReceiver>
@property (retain, nonatomic) IBOutlet UITextView *text;
@property (retain, nonatomic) NSLock* lock;
@property (retain, nonatomic) NSTimer* timer;
@property (retain, nonatomic) UMSubscriber* sub;
@property (retain, nonatomic) UMPublisher* pub;

@property (nonatomic) NSUInteger bytesRcvd;
@property (nonatomic) NSUInteger pktsDropped;
@property (nonatomic) NSUInteger pktsRecvd;
@property (nonatomic) NSUInteger lastSeqNr;

- (void)accumulateStats;
- (void)addText:(NSString*)theText;

@end
