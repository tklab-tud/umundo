//
//  UMNode.m
//  UMundoIOS
//
//  Created by Stefan Radomski on 11/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "UMNode.h"

@implementation UMNode

- (id) init {
  NSLog(@"Use UMNode::initWithDomain");
  return nil;
}

- (id) initWithDomain:(NSString*) domain {
  self = [super init];
  if(self) {
    if(domain == nil) {
      return nil;
    } else {
      std::string cppDomain([domain cStringUsingEncoding: NSASCIIStringEncoding]);
      _cppNode = boost::shared_ptr<umundo::Node>(new umundo::Node(cppDomain));
    }
  }
  return self;
}

- (void)addPublisher:(UMPublisher*)pub {
  _cppNode->addPublisher(pub->_cppPub.get());
}

- (void)addSubscriber:(UMSubscriber*)sub {
  _cppNode->addSubscriber(sub->_cppSub.get());
}

@end
