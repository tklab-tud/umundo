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

#ifndef UMTYPEDSUBSCRIBER_H_BJ2RZ8K5
#define UMTYPEDSUBSCRIBER_H_BJ2RZ8K5

#import <Foundation/Foundation.h>

#include <google/protobuf/message_lite.h>
#import <umundo/core.h>
#import <umundo-objc/core.h>
#import <umundo-objc/s11n.h>

@protocol UMTypedSubscriberReceiver
- (void)received:
(void*)obj andMeta:
(NSDictionary*)meta;
@end

class umundoTypedReceiverWrapper : public umundo::Receiver {
public:
	umundoTypedReceiverWrapper(id<UMTypedSubscriberReceiver> receiver) : _objcTypedReceiver(receiver) {}

	virtual void receive(umundo::Message* msg);

	std::map<std::string, google::protobuf::MessageLite*> _deserializers;
	id<UMTypedSubscriberReceiver> _objcTypedReceiver;

};

@interface UMTypedSubscriber :
UMSubscriber {
	@public
	id<UMTypedSubscriberReceiver> _typedReceiver;
	umundoTypedReceiverWrapper* _cListener;
}
- (id) initWithChannel:
(NSString*)name andReceiver:
(id<UMTypedSubscriberReceiver>)receiver;
- (void) registerType:
(NSString*)type withDeserializer:
(google::protobuf::MessageLite*)deserializer;
@end

#endif /* end of include guard: UMTYPEDSUBSCRIBER_H_BJ2RZ8K5 */
