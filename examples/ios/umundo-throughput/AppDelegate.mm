//
//  AppDelegate.m
//  uMundoIOS
//
//  Created by Stefan Radomski on 4/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "AppDelegate.h"

@implementation AppDelegate

@synthesize window = _window;
@synthesize node, disc, rtpSub, mcastSub, tcpSub, reporter, bytesRcvd, pktsDropped, pktsRecvd, lastSeqNr, lastTimeStamp, startedTimeStamp, currTimeStamp, firstTimeStamp, reportInterval, lock;

- (void)received:(NSData*)data withMeta:(NSDictionary*)meta {
	[lock lock];
	bytesRcvd += [data length];
	pktsRecvd++;
	
	UInt64 currSeqNr;
	memcpy(&currSeqNr, (char*)[data bytes] + 0, 8);
	currSeqNr = CFSwapInt64BigToHost(currSeqNr);
	
	memcpy(&currTimeStamp, (char*)[data bytes] + 8, 8);
	currTimeStamp = CFSwapInt64BigToHost(currTimeStamp);
	
	memcpy(&reportInterval, (char*)[data bytes] + 16, 4);
	reportInterval = CFSwapInt32BigToHost(reportInterval);
	
	if (firstTimeStamp == 0 || currSeqNr < lastSeqNr)
		firstTimeStamp = currTimeStamp;
	
	if (currSeqNr < lastSeqNr)
		lastSeqNr = 0;
	
	if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
		pktsDropped += currSeqNr - lastSeqNr;
	}
	lastSeqNr = currSeqNr;
	
	if (currTimeStamp - reportInterval >= lastTimeStamp) {
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

	std::ostringstream firstTimeSS;
	firstTimeSS << firstTimeStamp;

	std::ostringstream startTimeSS;
	startTimeSS << startedTimeStamp;

	umundo::Message* msg = new umundo::Message();
	msg->putMeta("pkts.rcvd", pktsRecvdSS.str());
	msg->putMeta("pkts.dropped", pktsDroppedSS.str());
	msg->putMeta("bytes.rcvd", bytesRcvdSS.str());
	msg->putMeta("last.timestamp", currTimeSS.str());
	msg->putMeta("hostname", umundo::Host::getHostname());
	msg->putMeta("last.seq", lastSeqNrSS.str());
	msg->putMeta("first.timestamp", firstTimeSS.str());
	msg->putMeta("started.timestamp", startTimeSS.str());
	
	
	[reporter sendMsg:msg];
	delete msg;
	
	pktsDropped = 0;
	pktsRecvd = 0;
	bytesRcvd = 0;
	
	[[_window rootViewController] performSelectorOnMainThread:@selector(clearText) withObject:nil waitUntilDone:NO];
	[[_window rootViewController] performSelectorOnMainThread:@selector(addText:) withObject:logMsg waitUntilDone:NO];

	[lock unlock];
	
}

- (void)dealloc
{
	[_window release];
	[super dealloc];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	// Override point for customization after application launch.
	disc = [[UMDiscovery alloc] init];
	node = [[UMNode alloc] init];

	tcpSub = [[UMSubscriber alloc] initTCP:@"throughput.tcp"
																receiver:self];
	
	mcastSub = [[UMSubscriber alloc] initMCast:@"throughput.mcast"
																		receiver:self
																		 mcastIP:@"224.1.2.3"
																		portBase:42142];
	
	rtpSub = [[UMSubscriber alloc] initRTP:@"throughput.rtp" receiver:self];
	
	reporter = [[UMPublisher alloc] initWithChannel:@"reports"];
	
	[node addSubscriber:tcpSub];
	[node addSubscriber:mcastSub];
	[node addSubscriber:rtpSub];
	[node addPublisher:reporter];

	[disc add:node];
	startedTimeStamp = umundo::Thread::getTimeStampMs();
	
	bytesRcvd = 0;
	lastSeqNr = 0;
	firstTimeStamp = 0;
	//  timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(accumulateStats) userInfo:nil repeats:YES];

  return YES;
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
  /*
   Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
   Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
   */
	[node removeSubscriber:tcpSub];
	[node removeSubscriber:mcastSub];
	[node removeSubscriber:rtpSub];
	[node removePublisher:reporter];
	
	[disc remove:node];
	firstTimeStamp = 0;
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
  /*
   Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
   If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
   */
	
//	[node removeSubscriber:tcpSub];
//	[node removeSubscriber:mcastSub];
//	[node removeSubscriber:rtpSub];
//	[node removePublisher:reporter];
//	
//	[disc remove:node];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
  /*
   Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
   */
	[disc add:node];
	startedTimeStamp = umundo::Thread::getTimeStampMs();

	[node addSubscriber:tcpSub];
	[node addSubscriber:mcastSub];
	[node addSubscriber:rtpSub];
	[node addPublisher:reporter];

}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  /*
   Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
   */
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  /*
   Called when the application is about to terminate.
   Save data if appropriate.
   See also applicationDidEnterBackground:.
   */
	[node removeSubscriber:tcpSub];
	[node removeSubscriber:mcastSub];
	[node removeSubscriber:rtpSub];
	[node removePublisher:reporter];

	[disc remove:node];
	firstTimeStamp = 0;
}

@end
