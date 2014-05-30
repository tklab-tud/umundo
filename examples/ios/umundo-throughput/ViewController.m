//
//  ViewController.m
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "ViewController.h"

@implementation ViewController
@synthesize text, timer, sub, pub, bytesRcvd, pktsDropped, pktsRecvd, lastSeqNr, lock;

- (void)addText:(NSString*)theText {
  text.text = [text.text stringByAppendingString:theText];
}

- (void)received:(NSData*)data withMeta:(NSDictionary*)meta {
	[lock lock];
	bytesRcvd += [data length];
	pktsRecvd++;
	
	NSString* seqNrStr = [meta valueForKey:@"seqNr"];
	NSUInteger currSeqNr = [seqNrStr integerValue];
	
	if (currSeqNr < lastSeqNr)
		lastSeqNr = 0;

	if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
		NSString *nrDropped = [NSString stringWithFormat:@"[!%d]", currSeqNr - lastSeqNr];
		[self performSelectorOnMainThread:@selector(addText:) withObject:nrDropped waitUntilDone:NO];
		pktsDropped += currSeqNr - lastSeqNr;
	}
	lastSeqNr = currSeqNr;
	[lock unlock];
}

- (void)accumulateStats {
	[lock lock];
	NSString *logMsg = [NSString stringWithFormat:@"[%ikB]", (NSUInteger)(bytesRcvd / 1024)];

	std::ostringstream bytesRcvdSS;
	bytesRcvdSS << bytesRcvd;

	std::ostringstream pktsDroppedSS;
	pktsDroppedSS << pktsDropped;

	std::ostringstream pktsRecvdSS;
	pktsRecvdSS << pktsRecvd;

	std::ostringstream lastSeqNrSS;
	lastSeqNrSS << lastSeqNr;

	umundo::Message* msg = new umundo::Message();
	msg->putMeta("bytes.rcvd", bytesRcvdSS.str());
	msg->putMeta("pkts.dropped", pktsDroppedSS.str());
	msg->putMeta("pkts.rcvd", pktsRecvdSS.str());
	msg->putMeta("last.seq", lastSeqNrSS.str());
	[pub sendMsg:msg];
	delete msg;
	
	pktsDropped = 0;
	pktsRecvd = 0;
	bytesRcvd = 0;

  [self performSelectorOnMainThread:@selector(addText:) withObject:logMsg waitUntilDone:NO];
	[lock unlock];

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
	bytesRcvd = 0;
	lastSeqNr = 0;
  timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(accumulateStats) userInfo:nil repeats:YES];
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
  sub = [[UMSubscriber alloc] initWithChannel:@"throughput" andReceiver:self];
  [[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addSubscriber:sub];
  pub = [[UMPublisher alloc] initWithChannel:@"reports"];
  [[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addPublisher:pub];

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
