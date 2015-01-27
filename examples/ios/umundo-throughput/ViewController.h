//
//  ViewController.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

@interface ViewController : UIViewController
@property (retain, nonatomic) IBOutlet UITextView *text;

- (void)addText:(NSString*)theText;
- (void)clearText;


@end
