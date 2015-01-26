#import <Foundation/Foundation.h>

#import "umundo-objc/core.h"
#import "umundo-objc/s11n.h"
#import "interfaces/protobuf/um_person.pb.h"

@interface TypedDataReceiver : NSObject<UMTypedSubscriberReceiver>
- (void)received:(void*)obj andMeta:(NSDictionary *)meta;
@end

@implementation TypedDataReceiver : NSObject
- (void)received:(void*)obj andMeta:(NSDictionary *)meta {
  NSLog(@"Received typed message\n");
  if ([@"person" isEqualToString:[meta valueForKey:@"type"]]) {
    Person* person = (Person*)obj;
    std::cout << person->name() << std::endl;
  }
}
@end

int main(int argc, char** argv) {
  @autoreleasepool {
    
    UMNode* node1 = [[UMNode alloc] initWithDomain:@""];
    UMNode* node2 = [[UMNode alloc] initWithDomain:@""];
    
    TypedDataReceiver* tRecv = [[TypedDataReceiver alloc] init];
    UMTypedPublisher* tPub = [[UMTypedPublisher alloc] initWithChannel:@"fooChannel"];
    UMTypedSubscriber* tSub = [[UMTypedSubscriber alloc] initWithChannel:@"fooChannel" andReceiver:tRecv];
    
    Person* person = new Person();
    person->set_id(24);
    person->set_name("Captain Foobar");
    
    [tSub registerType:@"person" withDeserializer:person];
    [node1 addPublisher:tPub];
    [node2 addSubscriber:tSub];
    
    while(true) {
      [NSThread sleepForTimeInterval:1.0];
      //NSLog(@"Not sending data");
      [tPub sendObj:person asType:@"person"];
    }
  }
}