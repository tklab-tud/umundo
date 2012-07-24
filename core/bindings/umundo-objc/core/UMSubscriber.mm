//
//  UMSubscriber.m
//  uMundoIOS
//
//  Created by Stefan Radomski on 14/1/12.
//  Copyright (c) 2012 TU Darmstadt. All rights reserved.
//

#import "UMSubscriber.h"
#import "umundo/core.h"

class umundoReceiverWrapper : public umundo::Receiver {
public:
  umundoReceiverWrapper(id<UMSubscriberReceiver> receiver) : _objcReceiver(receiver) {}
  id<UMSubscriberReceiver> _objcReceiver;
	virtual void receive(umundo::Message* msg) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSData* nsData = [[NSData alloc] initWithBytes:(msg->data()) length:msg->size()];
    NSMutableDictionary* nsMeta = [[NSMutableDictionary alloc] init];
    std::map<std::string, std::string>::const_iterator metaIter;
    for (metaIter = msg->getMeta().begin(); metaIter != msg->getMeta().end(); metaIter++) {
      [nsMeta 
       setValue:[NSString stringWithCString:metaIter->first.c_str() encoding:[NSString defaultCStringEncoding]] 
       forKey:[NSString stringWithCString:metaIter->second.c_str() encoding:[NSString defaultCStringEncoding]]];
    }
    [_objcReceiver received:nsData withMeta:nsMeta];
    [pool drain];

  }
};

@implementation UMSubscriber

- (id) init {
  self = [super init];
  return self;
}

- (id) initWithChannel:(NSString*)name andReceiver:(id<UMSubscriberReceiver>)listener {
  self = [super init];
  if(self) {
    if(name == nil || listener == nil) {
      return nil;
    } else {
      umundo::Receiver* cListener = new umundoReceiverWrapper(listener);
      std::string cppChannelName([name cStringUsingEncoding: NSASCIIStringEncoding]);
      _cppSub = 
        boost::shared_ptr<umundo::Subscriber>(new umundo::Subscriber(cppChannelName, cListener));
    }
  }
  return self;
}
@end
