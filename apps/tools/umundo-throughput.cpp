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
#include "umundo/core.h"
#include <iostream>
#include <iomanip>
#include <string.h>
#include <algorithm>    // std::max

#ifdef WIN32
#include "XGetopt.h"
#endif

#define FORMAT_COL std::setw(12) << std::left

using namespace umundo;

RMutex mutex;
bool isClient = false;
bool isServer = false;

// server
size_t bytesPerSecond = 1024 * 1024;
size_t mtu = 1280;
double pcntLossOk = 0;
size_t waitForSubs = 0;

// client
size_t bytesRcvd = 0;
size_t pktsRecvd = 0;
size_t lastSeqNr = 0;
size_t pktsDropped = 0;

class Report {
public:
	size_t pktsRcvd;
	size_t pktsDropped;
	size_t bytesRcvd;
	double pcntLoss;
	std::string name;
};

std::map<std::string, Report> reports;

class ThroughputReceiver : public Receiver {
	void receive(Message* msg) {
		RScopeLock lock(mutex);
		bytesRcvd += msg->size();
		pktsRecvd++;
		
		size_t currSeqNr = strTo<size_t>(msg->getMeta("seqNr"));
		
		if (currSeqNr < lastSeqNr)
			lastSeqNr = 0;
			
		if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
			pktsDropped += currSeqNr - lastSeqNr;
		}
		
		lastSeqNr = currSeqNr;
	}
};

class ReportReceiver : public Receiver {
public:
	ReportReceiver() {};
	void receive(Message* msg) {
		RScopeLock lock(mutex);
		
		std::string pubUUID = msg->getMeta("um.pub");
		
		Report& report = reports[pubUUID];
		
		report.pktsRcvd    = strTo<size_t>(msg->getMeta("pkts.rcvd"));
		report.pktsDropped = strTo<size_t>(msg->getMeta("pkts.dropped"));
		report.bytesRcvd   = strTo<size_t>(msg->getMeta("bytes.rcvd"));
		
		if (report.pktsDropped > 0) {
			double onePerc = (double)(report.pktsDropped + report.pktsRcvd) * 0.01;
			onePerc = (std::max)(onePerc, 0.0001);
			report.pcntLoss = (std::min)((double)report.pktsDropped / onePerc, 100.0);
		} else {
			report.pcntLoss = 0;
		}
		
		if (report.pcntLoss <= pcntLossOk) {
			bytesPerSecond *= 1.1;
		} else {
			bytesPerSecond *= 0.9;
		}
	}
};

class ThroughputGreeter : public Greeter {
	void welcome(Publisher&, const SubscriberStub&) {
		
	}
	void farewell(Publisher& pub, const SubscriberStub& subStub) {
		RScopeLock lock(mutex);
		if (reports.find(subStub.getUUID()) != reports.end())
			reports.erase(subStub.getUUID());
	}
};

void printUsageAndExit() {
	printf("umundo-throughput version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-throughput -s|-c [-l N] [-m MTU] [-w N]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c                 : act as a client\n");
	printf("\t-s                 : act as a server\n");
	printf("\t-l                 : acceptable packet loss in percent\n");
	printf("\t-m                 : MTU to use on server (defaults to 1280)\n");
	printf("\t-w <number>        : wait for given number of subscribers before testing\n");
	exit(1);
}

std::string bytesToDisplay(size_t nrBytes) {
	std::stringstream ss;
	ss << std::setprecision(5);
	
	if (nrBytes < 1024) {
		ss << nrBytes << "B";
		return ss.str();
	}
	if (nrBytes < 1024 * 1024) {
		ss << ((double)nrBytes / (double)1024) << "KB";
		return ss.str();
	}
	if (nrBytes < 1024 * 1024 * 1024) {
		ss << ((double)nrBytes / (double)(1024 * 1024)) << "MB";
		return ss.str();
	}
	ss << ((double)nrBytes / (double)(1024 * 1024 * 1024)) << "GB";
	return ss.str();
}

int main(int argc, char** argv) {
	int option;
	while ((option = getopt(argc, argv, "csm:w:l:")) != -1) {
		switch(option) {
			case 'c':
				isClient = true;
				break;
			case 's':
				isServer = true;
				break;
			case 'l':
				pcntLossOk = atof((const char*)optarg);
				break;
			case 'm':
				mtu = atoi((const char*)optarg);
				break;
			case 'w':
				waitForSubs = atoi((const char*)optarg);
				break;
			default:
				printUsageAndExit();
				break;
		}
	}

	if (!isClient && !isServer)
		printUsageAndExit();

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);

	if (isServer) {
		ThroughputGreeter tpGreeter;
		ReportReceiver reportRecv;
		Publisher pub("throughput", &tpGreeter);
		Subscriber sub("reports", &reportRecv);

		node.addPublisher(pub);
		node.addSubscriber(sub);
		pub.waitForSubscribers(waitForSubs);
		
		// remove some bytes for meta fields
		size_t dataSize = (std::max)((size_t)(mtu - 120), (size_t)0);
		char* data = (char*)malloc(dataSize);
		Message* msg = new Message(data, dataSize);

		unsigned long seqNr = 0;
		unsigned long bytesWritten = 0;
		unsigned long packetsWritten = 0;

		uint64_t now = Thread::getTimeStampMs();
		while(1) {
			msg->putMeta("seqNr", toStr(++seqNr));
			pub.send(msg);
			
			bytesWritten += dataSize;
			packetsWritten++;
			
			size_t packetsPerSecond = 0;
			size_t delay = 0;
			// sleep just enough to reach the desired bps
			{
				RScopeLock lock(mutex);
				packetsPerSecond = bytesPerSecond / dataSize;
				delay = (1000000000) / packetsPerSecond;
			}
			
			// every second we are writing reports
			if (Thread::getTimeStampMs() - now > 1000) {
				RScopeLock lock(mutex);
				
//				std::cout << FORMAT_COL << bytesToDisplay(bytesPerSecond) + "/s";
				std::cout << FORMAT_COL << bytesToDisplay(bytesWritten) + "/s";
				std::cout << FORMAT_COL << "#" + toStr(packetsWritten);
				std::cout << FORMAT_COL << toStr(delay) + "ns ---" << std::endl;

				if (reports.size() > 0) {
					
					std::cout << FORMAT_COL << "bytes recv";
					std::cout << FORMAT_COL << "pkts recv";
					std::cout << FORMAT_COL << "pkts loss";
					std::cout << FORMAT_COL << "% loss";
					std::cout << FORMAT_COL << "name" << std::endl;

					// print report messages
					std::map<std::string, Report>::iterator repIter = reports.begin();
					while(repIter != reports.end()) {
						const Report& report = repIter->second;
						std::cout << FORMAT_COL << bytesToDisplay(report.bytesRcvd) + "/s";
						std::cout << FORMAT_COL << toStr(report.pktsRcvd) + "pkt/s";
						std::cout << FORMAT_COL << toStr(report.pktsDropped) + "pkt/s";
						std::cout << FORMAT_COL << toStr(report.pcntLoss) + "%";
						if (repIter->first.size() >= 6)
							std::cout << FORMAT_COL << repIter->first.substr(0, 6);
						std::cout << std::endl;
						repIter++;
					}
					std::cout << std::endl;
				}
				
				bytesWritten = 0;
				packetsWritten = 0;
				reports.clear();
				
				now = Thread::getTimeStampMs();
			}
			Thread::sleepNs(delay);
		}
		delete(msg);

	} else {
		ThroughputReceiver tpRcvr;
		Subscriber sub("throughput", &tpRcvr);
		
		Publisher pub("reports");
		
		node.addSubscriber(sub);
		node.addPublisher(pub);
		
		while(true) {
			Thread::sleepMs(1000);
			{
				RScopeLock lock(mutex);
				Message* msg = new Message();
				msg->putMeta("bytes.rcvd", toStr(bytesRcvd));
				msg->putMeta("pkts.dropped", toStr(pktsDropped));
				msg->putMeta("pkts.rcvd", toStr(pktsRecvd));
				msg->putMeta("last.seq", toStr(lastSeqNr));
				pub.send(msg);
				delete msg;
				
				pktsDropped = 0;
				pktsRecvd = 0;
				bytesRcvd = 0;
			}
		}
	}
}
