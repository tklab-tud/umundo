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

#include "umundo/config.h"
#include "umundo.h"
#include <iostream>
#include <string.h>

using namespace umundo;

int main(int argc, char** argv) {
	uint32_t len=0;
	char buffer[4096];
	printf("umundo-stream-send version " UMUNDO_VERSION " (" UMUNDO_PLATFORM_ID " " CMAKE_BUILD_TYPE " build)\n");
	Publisher pubFoo("stream");
	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);

	FILE* fp=fopen(argv[1], "r");
	while(1) {
		Message* msg = new Message();
		fread(buffer, sizeof(buffer), 1, fp);
		msg->setData(buffer, sizeof(buffer));
		len+=sizeof(buffer);
		if(len>1024*1024) {			//output o every 1MB
			std::cout << "o" << std::flush;
			len=0;
		}
		pubFoo.send(msg);
		delete(msg);
	}
}
