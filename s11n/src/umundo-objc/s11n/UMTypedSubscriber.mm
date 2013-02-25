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

#include "UMTypedSubscriber.h"
#import "umundo/core.h"


@implementation UMTypedSubscriber

- (id) init {
  NSLog(@"Use UMSubscriber::initWithChannel:andListener");
  return nil;
}

- (id) initWithChannel:(NSString*)name andReceiver:(id<UMTypedSubscriberReceiver>)listener {
  self = [super init];
  if(self) {
    if(name == nil || listener == nil) {
      return nil;
    } else {
      _cListener = new umundoTypedReceiverWrapper(listener);
      std::string cppChannelName([name cStringUsingEncoding: NSASCIIStringEncoding]);
      _cppSub = boost::shared_ptr<umundo::Subscriber>(new umundo::Subscriber(cppChannelName, _cListener));
    }
  }
  return self;
}

- (void) registerType:(NSString*)type withDeserializer:(google::protobuf::MessageLite*)deserializer {
  _cListener->_deserializers[[type cStringUsingEncoding:NSASCIIStringEncoding]] = deserializer;
}

@end

void umundoTypedReceiverWrapper::receive(umundo::Message* msg) {
		// use an @autorelease {} here?
  @autoreleasepool {
    google::protobuf::MessageLite* pbObj = _deserializers[msg->getMeta("um.s11n.type")]->New();
    pbObj->ParseFromString(std::string(msg->data(), msg->size()));
    assert(pbObj->SerializeAsString().compare(std::string(msg->data(), msg->size())) == 0);
    NSMutableDictionary* meta = [[NSMutableDictionary alloc] init];

    std::map<std::string, std::string>::const_iterator metaIter;
    for (metaIter = msg->getMeta().begin(); metaIter != msg->getMeta().end(); metaIter++) {
      NSString* key = [NSString stringWithCString:metaIter->first.c_str() encoding:NSASCIIStringEncoding];
      NSString* value = [NSString stringWithCString:metaIter->second.c_str() encoding:NSASCIIStringEncoding];
      [meta setValue:value forKey:key];
    }
    [_objcTypedReceiver received:pbObj andMeta:meta];

  }
}

