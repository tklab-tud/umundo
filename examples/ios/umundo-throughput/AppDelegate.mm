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

/**
 * ENTERING
 * Lifecycle "fresh start"
 * 1. willFinishLaunchingWithOptions
 * 2. didFinishLaunchingWithOptions
 * 3. applicationDidBecomeActive
 *
 * Lifecycle "Reselect Task" / "Restart Running (from HomeScreen)"
 * 1. applicationWillEnterForeground
 * 2. applicationDidBecomeActive
 *
 * Lifecycle "Enter from TaskMgr <- other task"
 * 1. applicationWillEnterForeground
 * 2. applicationDidBecomeActive
 *
 * LEAVING
 * Lifecycle "Home Button"
 * 1. applicationWillResignActive
 * 2. applicationDidEnterBackground
 *
 * Lifecycle "Leave for TaskMgr -> other task"
 * 1. applicationWillResignActive
 * 2. applicationDidEnterBackground
 *
 * OTHER
 * Lifecycle "Leave for TaskMgr -> same task"
 * 1. applicationWillResignActive
 * 2. applicationDidBecomeActive
 *
 */

- (BOOL)application:(UIApplication *)application willFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	return YES;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// Override point for customization after application launch.
	node = [[UMNode alloc] init];
	disc = [[UMDiscovery alloc] init];

	tcpSub = [[UMSubscriber alloc] initTCP:@"throughput.tcp"
																receiver:self];
	
	mcastSub = [[UMSubscriber alloc] initMCast:@"throughput.mcast"
																		receiver:self
																		 mcastIP:@"224.1.2.3"
																		portBase:42142];
	
	rtpSub = [[UMSubscriber alloc] initRTP:@"throughput.rtp" receiver:self];
	
	reporter = [[UMPublisher alloc] initWithChannel:@"reports"];
	startedTimeStamp = umundo::Thread::getTimeStampMs();
	
	bytesRcvd = 0;
	lastSeqNr = 0;
	firstTimeStamp = 0;
	//  timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(accumulateStats) userInfo:nil repeats:YES];

	startedTimeStamp = umundo::Thread::getTimeStampMs();
	
	[node addSubscriber:tcpSub];
	[node addSubscriber:mcastSub];
	[node addSubscriber:rtpSub];
	[node addPublisher:reporter];
	[disc add:node];

  return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
}

- (void)applicationWillResignActive:(UIApplication *)application
{
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	[disc remove:node];
	[node removeSubscriber:tcpSub];
	[node removeSubscriber:mcastSub];
	[node removeSubscriber:rtpSub];
	[node removePublisher:reporter];
	
	firstTimeStamp = 0;

	[disc dealloc]; // <- this is important to reclaim our sockets later!
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	disc = [[UMDiscovery alloc] init]; // <- we have to reinstantiate on every wakeup
	startedTimeStamp = umundo::Thread::getTimeStampMs();
	
	[node addSubscriber:tcpSub];
	[node addSubscriber:mcastSub];
	[node addSubscriber:rtpSub];
	[node addPublisher:reporter];
	[disc add:node];

}


- (void)applicationWillTerminate:(UIApplication *)application
{
//	[node removeSubscriber:tcpSub];
//	[node removeSubscriber:mcastSub];
//	[node removeSubscriber:rtpSub];
//	[node removePublisher:reporter];
//
//	[disc remove:node];
//	firstTimeStamp = 0;
}

@end
