/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
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
 */

#include "UMTypedPublisher.h"

@implementation UMTypedPublisher

- (id) init {
  NSLog(@"Use UMPublisher::initWithChannel");
  return nil;
}

- (id) initWithChannel:(NSString*) channelName {
  self = [super initWithChannel:channelName];
  return self;
}

- (void)sendObj:(google::protobuf::MessageLite*)data asType:(NSString*)type {
  std::string buffer = data->SerializeAsString();
  umundo::Message* msg = new umundo::Message(buffer.data(), buffer.size());
  msg->setMeta("type", std::string([type cStringUsingEncoding: NSASCIIStringEncoding]));
  _cppPub->send(msg);
}

- (void)registerType:(NSString*)type withSerializer:(google::protobuf::MessageLite*)serializer {
  // nothing needed here for protobuf
}

@end