// this needs to be up here for the template
%include "../../../../core/src/umundo/common/ResultSet.h"

// Provide a nicer Java interface to STL containers
%template(StringArray) std::vector<std::string>;
%template(StringMap) std::map<std::string, std::string>;
%template(PublisherMap) std::map<std::string, umundo::Publisher>;
%template(SubscriberMap) std::map<std::string, umundo::Subscriber>;
%template(PublisherStubMap) std::map<std::string, umundo::PublisherStub>;
%template(SubscriberStubMap) std::map<std::string, umundo::SubscriberStub>;
%template(NodeStubMap) std::map<std::string, umundo::NodeStub>;
%template(EndPointResultSet) umundo::ResultSet<umundo::EndPoint>;
%template(EndPointArray) std::vector<umundo::EndPoint>;
%template(InterfaceArray) std::vector<umundo::Interface>;

//******************************
// Beautify Node class
//******************************
%extend umundo::Node {
	std::vector<std::string> getPubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, Publisher> pubs = self->getPublishers();
		std::map<std::string, Publisher>::iterator pubIter = pubs.begin();
		while(pubIter != pubs.end()) {
			keys.push_back(pubIter->first);
			pubIter++;
		}
		return keys;
	}
	std::vector<std::string> getSubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, Subscriber> subs = self->getSubscribers();
		std::map<std::string, Subscriber>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			keys.push_back(subIter->first);
			subIter++;
		}
		return keys;
	}
};

//******************************
// Beautify NodeStub class
//******************************
%extend umundo::NodeStub {
  std::vector<std::string> getPubKeys() {
  	std::vector<std::string> keys;
  	std::map<std::string, PublisherStub> pubs = self->getPublishers();
  	std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
  	while(pubIter != pubs.end()) {
  		keys.push_back(pubIter->first);
  		pubIter++;
  	}
  	return keys;
  }
  std::vector<std::string> getSubKeys() {
  	std::vector<std::string> keys;
  	std::map<std::string, SubscriberStub> subs = self->getSubscribers();
  	std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
  	while(subIter != subs.end()) {
  		keys.push_back(subIter->first);
  		subIter++;
  	}
  	return keys;
  }
};

//******************************
// Beautify Publisher class
//******************************

%extend umundo::Publisher {
	std::vector<std::string> getSubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, SubscriberStub> subs = self->getSubscribers();
		std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			keys.push_back(subIter->first);
			subIter++;
		}
		return keys;
	}
};

//******************************
// Beautify Subscriber class
//******************************

%extend umundo::Subscriber {
	std::vector<std::string> getPubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, PublisherStub> pubs = self->getPublishers();
		std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
		while(pubIter != pubs.end()) {
			keys.push_back(pubIter->first);
			pubIter++;
		}
		return keys;
	}
};

//******************************
// Beautify Message class
//******************************

%extend umundo::Message {
  std::vector<std::string> getMetaKeys() {
  	std::vector<std::string> keys;
  	std::map<std::string, std::string> metas = self->getMeta();
  	std::map<std::string, std::string>::iterator metaIter = metas.begin();
  	while(metaIter != metas.end()) {
  		keys.push_back(metaIter->first);
  		metaIter++;
  	}
  	return keys;
  }
};
