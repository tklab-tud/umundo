#include <zmq.h>
#include "umundo/core.h"

int main(int argc, char** argv) {
	void* context = zmq_ctx_new();
	void* subSocket;

	int32_t more;
	size_t more_size = sizeof(more);

	(subSocket = zmq_socket(context, ZMQ_SUB)) || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	zmq_connect(subSocket, "tcp://localhost:30101") && LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	zmq_setsockopt(subSocket, ZMQ_SUBSCRIBE, "foo", 3)  && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));

	while(1) {
		zmq_pollitem_t items [] = {
			{ subSocket,    0, ZMQ_POLLIN, 0 }, // read subscriptions
		};
		zmq_poll(items, 1, -1);

		if (items[0].revents & ZMQ_POLLIN) {
			while (1) {
				zmq_msg_t msg;
				zmq_msg_init (&msg);
				zmq_msg_recv (&msg, subSocket, 0);
				char* buffer = (char*)zmq_msg_data(&msg);

				std::cout << buffer << std::endl;

				zmq_getsockopt (subSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&msg);

				if (!more)
					break;      //  Last message part
			}
		}

	}

	zmq_ctx_destroy(context);
	return EXIT_SUCCESS;
}