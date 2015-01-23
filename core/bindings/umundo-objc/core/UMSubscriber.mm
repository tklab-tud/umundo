/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

#import "UMSubscriber.h"
#import "umundo/core.h"

class umundoReceiverWrapper : public umundo::Receiver {
public:
  umundoReceiverWrapper(id<UMReceiver> receiver) : _objcReceiver(receiver) {}
  id<UMReceiver> _objcReceiver;
	virtual void receive(umundo::Message* msg) {
#if HAS_AUTORELEASE_POOL
    @autoreleasepool {
#else
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#endif
      
      NSData* nsData = [[NSData alloc] initWithBytes:(msg->data()) length:msg->size()];
      NSMutableDictionary* nsMeta = [[NSMutableDictionary alloc] init];
      std::map<std::string, std::string>::const_iterator metaIter;
      for (metaIter = msg->getMeta().begin(); metaIter != msg->getMeta().end(); metaIter++) {
        [nsMeta
         setValue:[NSString stringWithCString:metaIter->second.c_str() encoding:[NSString defaultCStringEncoding]]
         forKey:[NSString stringWithCString:metaIter->first.c_str() encoding:[NSString defaultCStringEncoding]]];
      }
      [_objcReceiver received:nsData withMeta:nsMeta];
      [nsData release];
      [nsMeta release];
#if HAS_AUTORELEASE_POOL
    }
#else
    [pool drain];
#endif
  }
};

@implementation UMSubscriber

- (id) init {
  self = [super init];
  return self;
}

- (id) initTCP:(NSString*)name
			receiver:(id<UMReceiver>)receiver {

	self = [super init];
	if(self) {
		if(name == nil || receiver == nil) {
			return nil;
		} else {
			umundo::Receiver* cRecv = new umundoReceiverWrapper(receiver);
			std::string channel([name cStringUsingEncoding: NSASCIIStringEncoding]);
			_cppSub = SharedPtr<umundo::Subscriber>(new umundo::Subscriber(channel, cRecv));
		}
	}
	return self;
}

- (id) initRTP:(NSString*)name
			receiver:(id<UMReceiver>)receiver {
	self = [super init];
	if (self) {
		if (name == nil || receiver == nil) {
			return nil;
		} else {
			umundo::Receiver* cRecv = new umundoReceiverWrapper(receiver);
			std::string channel([name cStringUsingEncoding: NSASCIIStringEncoding]);
			umundo::SubscriberConfigRTP rtpConf(channel, cRecv);
			_cppSub = SharedPtr<umundo::Subscriber>(new umundo::Subscriber(&rtpConf));
		}
	}
	return self;

}

- (id) initMCast:(NSString*)name
				receiver:(id<UMReceiver>)receiver
				 mcastIP:(NSString*)ip
				portBase:(short)portBase {
	self = [super init];
	if (self) {
		if (name == nil || receiver == nil || ip == nil || portBase == 0) {
			return nil;
		} else {
			umundo::Receiver* cRecv = new umundoReceiverWrapper(receiver);
			std::string channel([name cStringUsingEncoding: NSASCIIStringEncoding]);
			umundo::SubscriberConfigMCast mcastConf(channel, cRecv);
			mcastConf.setMulticastIP([ip cStringUsingEncoding: NSASCIIStringEncoding]);
			mcastConf.setMulticastPortbase(portBase);
			_cppSub = SharedPtr<umundo::Subscriber>(new umundo::Subscriber(&mcastConf));
		}
	}
	return self;

}


@end
