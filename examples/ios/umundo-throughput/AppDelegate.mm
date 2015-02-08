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
@synthesize node, disc, rtpSub, mcastSub, tcpSub, reporter, lock, bytesRcvd, pktsRecvd, pktsDropped, lastSeqNr, timeStampServerLast, timeStampServerFirst, currReportNr, timeStampStartedAt, serverUUID;

- (void)received:(NSData*)data withMeta:(NSDictionary*)meta {
	[lock lock];
	
	UInt64 currSeqNr;
	memcpy(&currSeqNr, (char*)[data bytes] + 0, 8);
	currSeqNr = CFSwapInt64BigToHost(currSeqNr);
	
	UInt64 currServerTimeStamp;
	memcpy(&currServerTimeStamp, (char*)[data bytes] + 8, 8);
	currServerTimeStamp = CFSwapInt64BigToHost(currServerTimeStamp);
	
	UInt32 reportInterval;
	memcpy(&reportInterval, (char*)[data bytes] + 16, 4);
	reportInterval = CFSwapInt32BigToHost(reportInterval);
	
	if (currSeqNr < lastSeqNr) {
		// new throughput run!
		lastSeqNr = 0;
		timeStampServerFirst = 0;
		currReportNr = 0;
		bytesRcvd = 0;
		pktsRecvd = 0;
		pktsDropped = 0;
	}
	
	bytesRcvd += [data length];
	pktsRecvd++;
	
	if (timeStampServerFirst == 0)
		timeStampServerFirst = currServerTimeStamp;
	
	if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
		pktsDropped += currSeqNr - lastSeqNr;
	}
	
	lastSeqNr = currSeqNr;
	
	if (currServerTimeStamp - reportInterval >= timeStampServerLast) {
		timeStampServerLast = currServerTimeStamp;
		[self accumulateStats];
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
	
	umundo::Message* msg = new umundo::Message();
	msg->putMeta("bytes.rcvd", umundo::toStr(bytesRcvd));
	msg->putMeta("pkts.dropped", umundo::toStr(pktsDropped));
	msg->putMeta("pkts.rcvd", umundo::toStr(pktsRecvd));
	msg->putMeta("last.seq", umundo::toStr(lastSeqNr));
	msg->putMeta("report.seq", umundo::toStr(currReportNr++));
	msg->putMeta("timestamp.server.last", umundo::toStr(timeStampServerLast));
	msg->putMeta("timestamp.server.first", umundo::toStr(timeStampServerFirst));
	msg->putMeta("hostname", umundo::Host::getHostname());
	
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

- (void)stopUmundo {
	[node removeSubscriber:tcpSub];
	[node removeSubscriber:mcastSub];
	[node removeSubscriber:rtpSub];
	[node removePublisher:reporter];
	[disc remove:node];
	
	
	[disc dealloc]; // <- this is important to reclaim our sockets later!
	[node dealloc];
	[tcpSub dealloc];
	[mcastSub dealloc];
	[rtpSub dealloc];
	[reporter dealloc];
}

- (void)startUmundo {
	node = [[UMNode alloc] init];
	disc = [[UMDiscovery alloc] init];
	
	tcpSub = [[UMSubscriber alloc] initTCP:@"throughput.tcp"
																receiver:self];
	
	mcastSub = [[UMSubscriber alloc] initMCast:@"throughput.mcast"
																		receiver:self
																		 mcastIP:@"224.1.2.8"
																		portBase:22022];
	
	rtpSub = [[UMSubscriber alloc] initRTP:@"throughput.rtp" receiver:self];
	
	reporter = [[UMPublisher alloc] initWithChannel:@"reports"];

	[node addSubscriber:tcpSub];
	[node addSubscriber:mcastSub];
	[node addSubscriber:rtpSub];
	[node addPublisher:reporter];
	[disc add:node];

	timeStampStartedAt = umundo::Thread::getTimeStampMs();
}

- (BOOL)application:(UIApplication *)application willFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	return YES;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	// Override point for customization after application launch.
	
	[application setIdleTimerDisabled:YES];
	
	currReportNr = 0;
	bytesRcvd = 0;
	lastSeqNr = 0;
	timeStampServerFirst = 0;
	//  timer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(accumulateStats) userInfo:nil repeats:YES];

	[self startUmundo];

  return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
}

- (void)applicationWillResignActive:(UIApplication *)application
{
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	[self stopUmundo];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	[self startUmundo];
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
