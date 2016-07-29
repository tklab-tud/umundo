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
@property (retain, nonatomic) NSTimer* timer;
@property (retain, nonatomic) UMPublisher* pub;
@property (retain, nonatomic) UMSubscriber* sub;
@property (retain, nonatomic) NSData* data;

- (void)sendMessage;
- (void)addText:(NSString*)theText;

@end
