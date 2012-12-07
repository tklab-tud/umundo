#include <zmq.h>
#include "umundo/core.h"
#include "umundo/connection/zeromq/ZeroMQNode.h"

int main(int argc, char** argv) {
	void* context = zmq_ctx_new();
	void* pubSocket;

	int32_t more;
	size_t more_size = sizeof(more);

	(pubSocket = zmq_socket(context, ZMQ_XPUB))  || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	zmq_bind(pubSocket, "tcp://*:30101") && LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

	while(1) {
		zmq_pollitem_t items [] = {
			{ pubSocket,    0, ZMQ_POLLIN, 0 }, // read subscriptions
		};
		zmq_poll(items, 1, 500);

		if (items[0].revents & ZMQ_POLLIN) {
			while (1) {
				zmq_msg_t msg;
				zmq_msg_init (&msg);
				zmq_msg_recv (&msg, pubSocket, 0);
				int msgSize = zmq_msg_size(&msg);
				char* buffer = (char*)zmq_msg_data(&msg);

				if (buffer[0] == 0) {
					std::cout << "unsubscribing on " << std::string(buffer + 1, msgSize - 1) << std::endl;
				} else {
					std::cout << "subscribing on " << std::string(buffer + 1, msgSize - 1) << std::endl;
				}

				zmq_getsockopt (pubSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&msg);

				if (!more)
					break;      //  Last message part
			}
		}

		zmq_msg_t channelEnvlp;
		ZMQ_PREPARE_STRING(channelEnvlp, "foo", 3);
		zmq_sendmsg(pubSocket, &channelEnvlp, ZMQ_SNDMORE) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
		zmq_msg_close(&channelEnvlp) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

		zmq_msg_t message;
		ZMQ_PREPARE_STRING(message, "this is foo!", 12);
		zmq_sendmsg(pubSocket, &message, 0) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
		zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));


	}

	zmq_ctx_destroy(context);
	return EXIT_SUCCESS;
}