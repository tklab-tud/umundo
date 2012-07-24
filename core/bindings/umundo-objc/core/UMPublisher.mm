//
//  uPublisher.m
//  uMundoIOS
//
//  Created by Stefan Radomski on 14/1/12.
//  Copyright (c) 2012 TU Darmstadt. All rights reserved.
//

#import "UMPublisher.h"

@implementation UMPublisher

- (id) init {
  NSLog(@"Use UMPublisher::initWithChannel");
  return nil;
}

- (id) initWithChannel:(NSString*) channelName {
  self = [super init];
  if(self) {
    if(channelName == nil) {
      return nil;
    } else {
      std::string cppChannelName([channelName cStringUsingEncoding: NSASCIIStringEncoding]);
      _cppPub = boost::shared_ptr<umundo::Publisher>(new umundo::Publisher(cppChannelName));
    }
  }
  return self;
}

- (void)sendMsg:(umundo::Message*)msg {
  _cppPub->send(msg);
}

- (void)send:(NSData*)data {
  _cppPub->send((char*)data.bytes, data.length);
}

@end
