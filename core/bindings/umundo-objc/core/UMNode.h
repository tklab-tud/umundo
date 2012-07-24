//
//  UMNode.h
//  UMundoIOS
//
//  Created by Stefan Radomski on 11/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef UMNODE_H_9C2MCZDC
#define UMNODE_H_9C2MCZDC

#import <Foundation/Foundation.h>

#include <string>
#include <boost/shared_ptr.hpp>

#include <umundo/core.h>
#include <umundo-objc/core/UMPublisher.h>
#include <umundo-objc/core/UMSubscriber.h>

@interface UMNode :
NSObject {
	boost::shared_ptr<umundo::Node> _cppNode;
}

- (id) initWithDomain:
(NSString*) domain;
- (void)addPublisher:
(UMPublisher*)pub;
- (void)addSubscriber:
(UMSubscriber*)sub;

@end

#endif /* end of include guard: UMNODE_H_9C2MCZDC */
