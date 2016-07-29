#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

#include <unistd.h> // usleep
#include <stdio.h> // printf
#include <stdlib.h> // malloc
#include <string.h> // strdup

#include <map>
#include <sstream>
#include <umundo.h>

template <typename T> std::string toStr(T tmp) {
	std::ostringstream out;
	out << tmp;
	return out.str();
}

AvahiSimplePoll *_simplePoll = NULL;
umundo::Mutex mutex;
umundo::Monitor monitor;

struct service {
	AvahiEntryGroup* group;
	const char* name;
	const char* transport;
	const char* domain;
	uint16_t port;
	const char* regType;
};

struct query {
	const char* domain;
	const char* regType;
};

std::map<service*, AvahiClient*> clients;
std::map<query*, AvahiClient*> queries;
std::map<std::string, service*> finds;

void resolveCallback(
    AvahiServiceResolver *r,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata
) {

	query* q = (query*)userdata;
	(void)q;

	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		printf("resolveCallback AVAHI_RESOLVER_FAILURE: %s\n", avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
		break;
	case AVAHI_RESOLVER_FOUND: {
		char addr[AVAHI_ADDRESS_STR_MAX], *t;
		t = avahi_string_list_to_string(txt);
		avahi_address_snprint(addr, sizeof(addr), address);
		if (protocol == AVAHI_PROTO_INET) {
			printf("resolveCallback ipv4 for iface %d: %s\n", interface, addr);
			char* start = addr;
			int dots = 0;
			while ((start = strchr(++start, '.')) != NULL) {
				dots++;
			}
			if (dots != 3) {
				printf("Avahi bug: %s is not an ipv4 address!\n", addr);
				assert(false);
			}
		} else if (protocol == AVAHI_PROTO_INET6) {
			printf("resolveCallback ipv6 for iface %d: %s\n", interface, addr);
		} else {
			printf("protocol is neither ipv4 nor ipv6\n");
		}
		avahi_free(t);
		break;
	}
	default:
		printf("Unknown event in resolveCallback\n");
	}

	avahi_service_resolver_free(r);
	monitor.signal();
}

void browseCallback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void* userdata
) {

	query* q = (query*)userdata;
	(void)q;

	switch (event) {
	case AVAHI_BROWSER_NEW: {
		printf("browseCallback AVAHI_BROWSER_NEW: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		avahi_service_resolver_new(
		    avahi_service_browser_get_client(b),
		    interface,
		    protocol,
		    name,
		    type,
		    domain,
		    AVAHI_PROTO_UNSPEC,
		    (AvahiLookupFlags)0, resolveCallback, userdata);
		break;
	}
	case AVAHI_BROWSER_REMOVE: {
		printf("browseCallback AVAHI_BROWSER_REMOVE: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		break;
	}
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		printf("browseCallback AVAHI_BROWSER_CACHE_EXHAUSTED: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		break;
	case AVAHI_BROWSER_ALL_FOR_NOW: {
		printf("browseCallback AVAHI_BROWSER_ALL_FOR_NOW: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		break;
	}
	case AVAHI_BROWSER_FAILURE:
		printf("browseCallback AVAHI_BROWSER_FAILURE: %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		break;
	}
}

void browseClientCallback(AvahiClient* c, AvahiClientState state, void* userdata) {
	assert(c);

	switch(state) {
	case AVAHI_CLIENT_CONNECTING:
		printf("browseClientCallback AVAHI_CLIENT_CONNECTING: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_FAILURE:
		printf("browseClientCallback AVAHI_CLIENT_FAILURE: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_RUNNING: {
		printf("browseClientCallback AVAHI_CLIENT_S_RUNNING: %s\n", avahi_strerror(avahi_client_errno(c)));
		query* q = (query*)userdata;
		AvahiServiceBrowser* sb = avahi_service_browser_new(
		                              c,
		                              AVAHI_IF_UNSPEC,
		                              AVAHI_PROTO_UNSPEC,
		                              q->regType,
		                              q->domain,
		                              (AvahiLookupFlags)0,
		                              browseCallback,
		                              (void*)q);
		if (sb != NULL) {
			queries[q] = c;
		} else {
			printf("browseClientCallback avahi_service_browser_new: %s\n", avahi_strerror(avahi_client_errno(c)));
		}

		break;
	}
	case AVAHI_CLIENT_S_REGISTERING:
		printf("browseClientCallback AVAHI_CLIENT_S_REGISTERING: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_COLLISION:
		printf("browseClientCallback AVAHI_CLIENT_S_COLLISION: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	default:
		printf("Unknown state");
		break;
	}
}

void entryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state, void* userdata) {
	service* svc = (service*)userdata;

	/* Called whenever the entry group state changes */
	switch (state) {
	case AVAHI_ENTRY_GROUP_ESTABLISHED :
		printf("entryGroupCallback state AVAHI_ENTRY_GROUP_ESTABLISHED\n");
		svc->group = g;
		break;
	case AVAHI_ENTRY_GROUP_COLLISION :
		printf("entryGroupCallback state AVAHI_ENTRY_GROUP_COLLISION: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;
	case AVAHI_ENTRY_GROUP_FAILURE :
		printf("entryGroupCallback state AVAHI_ENTRY_GROUP_FAILURE: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;
	case AVAHI_ENTRY_GROUP_UNCOMMITED:
		printf("entryGroupCallback state AVAHI_ENTRY_GROUP_UNCOMMITED: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;
	case AVAHI_ENTRY_GROUP_REGISTERING:
		printf("entryGroupCallback state AVAHI_ENTRY_GROUP_REGISTERING\n");
		break;
	default:
		printf("entryGroupCallback default switch should not happen!\n");
	}
}

void clientCallback(AvahiClient* c, AvahiClientState state, void* userdata) {
	int err;
	(void) err;
	service* svc = (service*)userdata;

	switch (state) {
	case AVAHI_CLIENT_S_RUNNING:
		printf("clientCallback AVAHI_CLIENT_S_RUNNING: %s\n", avahi_strerror(avahi_client_errno(c)));
		if (!svc->group) {
			svc->group = avahi_entry_group_new(c, entryGroupCallback, userdata);
			assert(svc->group);
		}
		if (avahi_entry_group_is_empty(svc->group)) {
			err = avahi_entry_group_add_service(
			          svc->group,
			          AVAHI_IF_UNSPEC,
			          AVAHI_PROTO_UNSPEC,
			          (AvahiPublishFlags)0,
			          svc->name,
			          svc->regType,
			          svc->domain,
			          NULL,
			          svc->port,
			          NULL);
			assert(err == 0);
			err = avahi_entry_group_commit(svc->group);
			assert(err == 0);
		}
		break;

	case AVAHI_CLIENT_FAILURE:
		printf("clientCallback AVAHI_CLIENT_FAILURE: %s\n", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(_simplePoll);
		break;
	case AVAHI_CLIENT_S_COLLISION:
		printf("clientCallback AVAHI_CLIENT_S_COLLISION: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_REGISTERING:
		printf("clientCallback AVAHI_CLIENT_S_REGISTERING\n");
		if (svc->group) {
			err = avahi_entry_group_reset(svc->group);
			assert(err == 0);
		}
		break;
	case AVAHI_CLIENT_CONNECTING:
		printf("clientCallback AVAHI_CLIENT_CONNECTING: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	}
}

class AvahiRunner : public umundo::Thread {
	void run() {
		while(isStarted()) {
			{
				umundo::ScopeLock lock(mutex);
				int timeoutMs = 50;
				avahi_simple_poll_iterate(_simplePoll, timeoutMs);
			}
			umundo::Thread::sleepMs(20);
		}
	}
};

int main(int argc, char** argv) {
	int err;

	AvahiClient *client = NULL;
	(void)client;

	(_simplePoll = avahi_simple_poll_new()) || printf("avahi_simple_poll_new\n");
	assert(_simplePoll);

	AvahiRunner* avahiRunner = new AvahiRunner();
	avahiRunner->start();

	struct query* q = (query*)malloc(sizeof(query));
	q->domain = "foo.local.";
	q->regType = "_foo._tcp";
	client = avahi_client_new(avahi_simple_poll_get(_simplePoll), (AvahiClientFlags)0, browseClientCallback, (void*)q, &err);

	while(true) {
		// int timeoutMs = 10;
		// avahi_simple_poll_iterate(_simplePoll, timeoutMs);

		for (int i = 0; i < 10; i++) {
			umundo::ScopeLock lock(mutex);
			struct service* svc = (service*)malloc(sizeof(service));
			svc->group = NULL;
			svc->name = strdup(toStr(i).c_str());
			svc->transport = "tcp";
			svc->domain = "foo.local.";
			svc->port = i + 40;
			svc->regType = "_foo._tcp";

			AvahiClient* client = avahi_client_new(avahi_simple_poll_get(_simplePoll), (AvahiClientFlags)0, &clientCallback, (void*)svc, &err);
			clients[svc] = client;
		}

		break;
	}

	umundo::Thread::sleepMs(6000);

	return 0;
}