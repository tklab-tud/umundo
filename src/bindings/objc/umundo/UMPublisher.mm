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

#ifndef __has_extension
#define __has_extension __has_feature
#endif

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
      _cppPub = SharedPtr<umundo::Publisher>(new umundo::Publisher(cppChannelName));
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
