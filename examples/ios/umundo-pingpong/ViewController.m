//
//  ViewController.m
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "ViewController.h"

@implementation ViewController
@synthesize text, timer, pub, sub, data;

- (void)addText:(NSString*)theText {
  text.text = [text.text stringByAppendingString:theText];
}

- (void)received:(NSData*)data withMeta:(NSDictionary*)meta {
  [self performSelectorOnMainThread:@selector(addText:) withObject:@"i" waitUntilDone:NO];
}

- (void)sendMessage {
  [pub send:data];
  [self performSelectorOnMainThread:@selector(addText:) withObject:@"o" waitUntilDone:NO];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
  [super viewDidLoad];
  text.text = @"";
  char* buffer = (char*)malloc(256);
  data = [[NSData alloc] initWithBytesNoCopy:buffer length:256];
  pub = [[UMPublisher alloc] initWithChannel:@"pingpong"];
  timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(sendMessage) userInfo:nil repeats:YES];
  
  [[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addPublisher:pub];
}

- (void)viewDidUnload
{
  [self setText:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
  sub = [[UMSubscriber alloc] initWithChannel:@"pingpong" andReceiver:self];
  [[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addSubscriber:sub];

}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
	[super viewDidDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
  if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
      return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
  } else {
      return YES;
  }
}

- (void)dealloc {
  [text release];
  [super dealloc];
}
@end
