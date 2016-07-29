//
//  AppDelegate.h
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import <umundo/s11n-objc.h>
#import <umundo/core-objc.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) UMNode *node;
@property (strong, nonatomic) UMDiscovery *disc;

@end
