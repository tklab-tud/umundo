//
//  ViewController.m
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "ViewController.h"

@implementation ViewController
@synthesize text, timer, rtpSub, mcastSub, tcpSub, reporter, bytesRcvd, pktsDropped, pktsRecvd, lastSeqNr, lastTimeStamp, currTimeStamp, lock;

- (void)addText:(NSString*)theText {
  text.text = [text.text stringByAppendingString:theText];
}

- (void)clearText {
	text.text = @"";
}


- (void)received:(NSData*)data withMeta:(NSDictionary*)meta {
	[lock lock];
	bytesRcvd += [data length];
	pktsRecvd++;
	
	UInt64 currSeqNr;
	memcpy(&currSeqNr, (char*)[data bytes] + 0, 8);
	currSeqNr = CFSwapInt64BigToHost(currSeqNr);

	memcpy(&currTimeStamp, (char*)[data bytes] + 8, 8);
	currTimeStamp = CFSwapInt64BigToHost(currTimeStamp);

	if (currSeqNr < lastSeqNr)
		lastSeqNr = 0;

	if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
		pktsDropped += currSeqNr - lastSeqNr;
	}
	lastSeqNr = currSeqNr;
	
	if (currTimeStamp - 1000 >= lastTimeStamp) {
		[self accumulateStats];
		lastTimeStamp = currTimeStamp;
	}
	
	[lock unlock];
}

- (void)accumulateStats {
	[lock lock];
	NSMutableString *logMsg = [[NSMutableString alloc] init];
	[logMsg appendString:@"\n\n"];
	[logMsg appendString:[NSString stringWithFormat:@"Byte rcvd: %ikB\n", (NSUInteger)(bytesRcvd / 1024)]];
	[logMsg appendString:[NSString stringWithFormat:@"Pkts rcvd: %i\n", pktsRecvd]];
	[logMsg appendString:[NSString stringWithFormat:@"Pkts drop: %i\n", pktsDropped]];
	[logMsg appendString:[NSString stringWithFormat:@"Last Seq:  %lld\n", lastSeqNr]];
	
	std::ostringstream bytesRcvdSS;
	bytesRcvdSS << bytesRcvd;

	std::ostringstream pktsDroppedSS;
	pktsDroppedSS << pktsDropped;

	std::ostringstream pktsRecvdSS;
	pktsRecvdSS << pktsRecvd;

	std::ostringstream lastSeqNrSS;
	lastSeqNrSS << lastSeqNr;

	std::ostringstream currTimeSS;
	currTimeSS << currTimeStamp;

	umundo::Message* msg = new umundo::Message();
	msg->putMeta("bytes.rcvd", bytesRcvdSS.str());
	msg->putMeta("pkts.dropped", pktsDroppedSS.str());
	msg->putMeta("pkts.rcvd", pktsRecvdSS.str());
	msg->putMeta("last.seq", lastSeqNrSS.str());
	msg->putMeta("last.timestamp", currTimeSS.str());

	[reporter sendMsg:msg];
	delete msg;
	
	pktsDropped = 0;
	pktsRecvd = 0;
	bytesRcvd = 0;

	[self performSelectorOnMainThread:@selector(clearText) withObject:nil waitUntilDone:NO];
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
//  timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(accumulateStats) userInfo:nil repeats:YES];
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
	
	tcpSub = [[UMSubscriber alloc] initTCP:@"throughput.tcp"
																receiver:self];
	
	mcastSub = [[UMSubscriber alloc] initMCast:@"throughput.mcast"
																		receiver:self
																		 mcastIP:@"224.1.2.3"
																		portBase:42142];

	rtpSub = [[UMSubscriber alloc] initRTP:@"throughput.rtp" receiver:self];

	reporter = [[UMPublisher alloc] initWithChannel:@"reports"];

	[[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addSubscriber:tcpSub];
	[[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addSubscriber:mcastSub];
	[[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addSubscriber:rtpSub];
  [[(AppDelegate *)[[UIApplication sharedApplication] delegate] node] addPublisher:reporter];

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
