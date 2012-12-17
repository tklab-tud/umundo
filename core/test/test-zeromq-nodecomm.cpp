#include <zmq.h>
#include <string.h>
#include <iostream>
#include <assert.h>

/// Initialize a zeromq message with a given null-terminated string
#define ZMQ_PREPARE_STRING(msg, data, size) \
zmq_msg_init(&msg) && printf("zmq_msg_init: %s\n", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size + 1) && printf("zmq_msg_init_size: %s\n",zmq_strerror(errno)); \
memcpy(zmq_msg_data(&msg), data, size + 1);

int main(int argc, char** argv) {
	void* context = zmq_ctx_new();
	void* routerSocket;
	void* dealerSocket;

	(routerSocket = zmq_socket(context, ZMQ_ROUTER))         || printf("zmq_socket: %s\n", zmq_strerror(errno));

	zmq_bind(routerSocket, "tcp://*:30010")                     && printf("zmq_bind: %s\n", zmq_strerror(errno));
	zmq_setsockopt(routerSocket, ZMQ_IDENTITY, "routerfoo", 9)  && printf("zmq_setsockopt: %s\n", zmq_strerror(errno));

	int more;
	size_t more_size = sizeof(more);
	int iteration = 0;

	while(1) {
		zmq_pollitem_t items [] = {
			{ routerSocket,    0, ZMQ_POLLIN, 0 },
		};
		zmq_poll(items, 1, 500);

		if (items[0].revents & ZMQ_POLLIN) {
			while (1) {
				zmq_msg_t msg;
				zmq_msg_init (&msg);
				zmq_msg_recv (&msg, routerSocket, 0);
				int msgSize = zmq_msg_size(&msg);
				char* buffer = (char*)zmq_msg_data(&msg);

				printf("router received '%s'\n", std::string(buffer, msgSize).c_str());

				zmq_getsockopt (routerSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&msg);

				if (!more)
					break;      //  Last message part
			}
		}

		if (items[1].revents & ZMQ_POLLIN) {
			while (1) {
				zmq_msg_t msg;
				zmq_msg_init (&msg);
				zmq_msg_recv (&msg, dealerSocket, 0);
				int msgSize = zmq_msg_size(&msg);
				char* buffer = (char*)zmq_msg_data(&msg);

				printf("dealer received '%s'\n", std::string(buffer, msgSize).c_str());

				zmq_getsockopt (dealerSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&msg);

				if (!more)
					break;      //  Last message part
			}
		}

		if (iteration == 1) {
			(dealerSocket = zmq_socket(context, ZMQ_DEALER))         || printf("zmq_socket: %s\n", zmq_strerror(errno));
			zmq_setsockopt(dealerSocket, ZMQ_IDENTITY, "dealerfoo", 9)  && printf("zmq_setsockopt: %s\n", zmq_strerror(errno));
			zmq_connect(dealerSocket, "tcp://127.0.0.1:30010") && printf("zmq_connect: %s", zmq_strerror(errno));
			zmq_msg_t msg;
			ZMQ_PREPARE_STRING(msg, "foo", 3);
			zmq_sendmsg(dealerSocket, &msg, 0);
			zmq_msg_close(&msg) && printf("zmq_msg_close: %s\n",zmq_strerror(errno));
		}

		if (iteration == 3) {
			zmq_close(dealerSocket) && printf("zmq_close: %s\n", zmq_strerror(errno));
		}

		if (iteration == 5) {
			(dealerSocket = zmq_socket(context, ZMQ_DEALER))         || printf("zmq_socket: %s\n", zmq_strerror(errno));
			zmq_setsockopt(dealerSocket, ZMQ_IDENTITY, "dealerbar", 9)  && printf("zmq_setsockopt: %s\n", zmq_strerror(errno));
			zmq_connect(dealerSocket, "tcp://127.0.0.1:30010") && printf("zmq_connect: %s", zmq_strerror(errno));
			zmq_msg_t msg;
			ZMQ_PREPARE_STRING(msg, "bar", 3);
			zmq_sendmsg(dealerSocket, &msg, 0);
			zmq_msg_close(&msg) && printf("zmq_msg_close: %s\n",zmq_strerror(errno));
			
		}

		if (iteration == 10) {
			break;
		}

		iteration++;
	}

	zmq_close(dealerSocket) && printf("zmq_close: %s", zmq_strerror(errno));
	zmq_close(routerSocket) && printf("zmq_close: %s", zmq_strerror(errno));

	zmq_ctx_destroy(context);
	return 0;
}