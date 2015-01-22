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

// see http://stackoverflow.com/questions/9695720/how-do-i-convert-a-64bit-integer-to-a-char-array-and-back
uint64_t charTo64bitNum(const char* a) {
	uint64_t n = 0;
	n
	= (((int64_t)a[0] << 56) & 0xFF00000000000000U)
	| (((int64_t)a[1] << 48) & 0x00FF000000000000U)
	| (((int64_t)a[2] << 40) & 0x0000FF0000000000U)
	| (((int64_t)a[3] << 32) & 0x000000FF00000000U)
	| ((a[4] << 24) & 0x00000000FF000000U)
	| ((a[5] << 16) & 0x0000000000FF0000U)
	| ((a[6] <<  8) & 0x000000000000FF00U)
	| (a[7]        & 0x00000000000000FFU);
	return n;
}

void uint64ToChar(char a[], uint64_t num) {
	for(int i = 0; i < 8; i++) a[i] = num >> (8-1-i)*8;
}


enum PubType {
	PUB_RTP,
	PUB_TCP,
	PUB_MCAST
};

RMutex mutex;
bool isClient = false;
bool isServer = false;
uint64_t currSeqNr = 0;

// server
size_t bytesPerSecond = 1024 * 1024;
size_t fixedBytesPerSecond = 0;
size_t mtu = 1280;
double pcntLossOk = 0;
size_t waitForSubs = 0;
size_t packetsWritten = 0;
PubType type = PUB_TCP;

// client
size_t bytesRcvd = 0;
size_t pktsRecvd = 0;
size_t lastSeqNr = 0;
size_t lastTimeStamp = 0;
size_t pktsDropped = 0;
Publisher reporter;

class Report {
public:
	size_t pktsRcvd;
	size_t pktsDropped;
	size_t bytesRcvd;
	size_t pktsLate;
	size_t latency;
	double pcntLoss;
	std::string name;
};

std::map<std::string, Report> reports;

class ThroughputReceiver : public Receiver {
	void receive(Message* msg) {
		RScopeLock lock(mutex);
		bytesRcvd += msg->size();
		pktsRecvd++;
		
		currSeqNr = charTo64bitNum(msg->data());
		uint64_t currTimeStamp = charTo64bitNum(msg->data() + 8);
		
		if (currSeqNr < lastSeqNr)
			lastSeqNr = 0;
			
		if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
			pktsDropped += currSeqNr - lastSeqNr;
		}
		
		lastSeqNr = currSeqNr;
		
		// reply?
		if (currTimeStamp - 1000 >= lastTimeStamp)
		{
			RScopeLock lock(mutex);
			Message* msg = new Message();
			msg->putMeta("bytes.rcvd", toStr(bytesRcvd));
			msg->putMeta("pkts.dropped", toStr(pktsDropped));
			msg->putMeta("pkts.rcvd", toStr(pktsRecvd));
			msg->putMeta("last.seq", toStr(lastSeqNr));
			msg->putMeta("last.timestamp", toStr(currTimeStamp));
			reporter.send(msg);
			delete msg;
			
			lastTimeStamp = currTimeStamp;
			pktsDropped = 0;
			pktsRecvd = 0;
			bytesRcvd = 0;
		}

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
		report.latency     = Thread::getTimeStampMs() - strTo<size_t>(msg->getMeta("last.timestamp"));
		
		size_t lastSeqNr   = strTo<size_t>(msg->getMeta("last.seq"));
		report.pktsLate = currSeqNr - lastSeqNr;
			
		if (report.pktsDropped > 0) {
			double onePerc = (double)(report.pktsDropped + report.pktsRcvd) * 0.01;
			onePerc = (std::max)(onePerc, 0.0001);
			report.pcntLoss = (std::min)((double)report.pktsDropped / onePerc, 100.0);
		} else {
			report.pcntLoss = 0;
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
	printf("\tumundo-throughput -s|-c [-l N] [-m N] [-w N]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c                 : act as a client\n");
	printf("\t-s                 : act as a server\n");
	printf("\t-f BYTES/s         : try to maintain given throughput\n");
	printf("\t-t [rtp|tcp|mcast] : type of publisher to measure throughput\n");
	printf("\t-l                 : acceptable packet loss in percent\n");
	printf("\t-m <number>        : MTU to use on server (defaults to 1280)\n");
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

std::string timeToDisplay(size_t ns) {
	std::stringstream ss;
	ss << std::setprecision(5);
	
	if (ns < 1000) {
		ss << ns << "ns";
		return ss.str();
	}
	if (ns < 1000 * 1000) {
		ss << ((double)ns / (double)1000) << "us";
		return ss.str();
	}
	if (ns < 1000 * 1000 * 1000) {
		ss << ((double)ns / (double)(1000 * 1000)) << "ms";
		return ss.str();
	}
	ss << ((double)ns / (double)(1000 * 1000 * 1000)) << "s";
	return ss.str();
}

size_t displayToBytes(const std::string& value) {
	size_t multiply = 1;
	char suffix = value[value.length() - 1];
	if (suffix == 'K' || suffix == 'k')
		multiply = 1024;
	if (suffix == 'M' || suffix == 'm')
		multiply = 1024 * 1024;
	if (suffix == 'G' || suffix == 'g')
		multiply = 1024 * 1024 * 1024;
	size_t result = atol((multiply > 1 ? value.substr(0, value.length() - 1).c_str() : value.c_str()));
	result *= multiply;
	return result;
}

int main(int argc, char** argv) {
	int option;
	while ((option = getopt(argc, argv, "csm:w:l:f:t:")) != -1) {
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
			case 't':
				if (strncasecmp(optarg, "tcp", 3) == 0) {
					type = PUB_TCP;
				} else if (strncasecmp(optarg, "mcast", 5) == 0) {
					type = PUB_MCAST;
				} else if (strncasecmp(optarg, "rtp", 3) == 0) {
					type = PUB_RTP;
				}
				break;
			case 'm':
				mtu = displayToBytes(optarg);
				break;
			case 'f':
				fixedBytesPerSecond = displayToBytes(optarg);
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

	if (fixedBytesPerSecond > 0)
		bytesPerSecond = fixedBytesPerSecond;
	
	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);

	if (isServer) {
		ThroughputGreeter tpGreeter;
		ReportReceiver reportRecv;
		
		Publisher pub;
		switch (type) {
			case PUB_RTP: {
				RTPPublisherConfig config = RTPPublisherConfig(166, 0);
				pub = Publisher(Publisher::RTP, "throughput.rtp", &config);
				pub.setGreeter(&tpGreeter);
				break;
			}
			case PUB_MCAST: {
				RTPPublisherConfig config = RTPPublisherConfig(166, 0);
				config.setMulticastIP("224.1.2.3");
				config.setMulticastPortbase(42042);
				pub = Publisher(Publisher::RTP, "throughput.mcast", &config);
				pub.setGreeter(&tpGreeter);
				break;
			}
			case PUB_TCP: {
				pub = Publisher("throughput.tcp");
				pub.setGreeter(&tpGreeter);
				break;
			}
		}
		
		Subscriber sub("reports", &reportRecv);

		node.addPublisher(pub);
		node.addSubscriber(sub);
		pub.waitForSubscribers(waitForSubs);
		
		// reserve 16 bytes for timestamp and sequence number
		size_t dataSize = (std::max)(mtu, (size_t)16);
		char* data = (char*)malloc(dataSize);
		Message* msg = new Message();

		unsigned long bytesWritten = 0;

		uint64_t lastReportAt = Thread::getTimeStampMs();
		while(1) {
			uint64_t now = Thread::getTimeStampMs();
			
			// first 16 bytes are seqNr and timestamp
			uint64ToChar(&data[0], ++currSeqNr);
			uint64ToChar(&data[8], now);
			msg->setData(data, dataSize);
			pub.send(msg);
			
			bytesWritten += dataSize;
			packetsWritten++;
			
			size_t delay = 0;
			// sleep just enough to reach the desired bps
			{
				RScopeLock lock(mutex);
				size_t packetsPerSecond = (std::max)(bytesPerSecond / dataSize, (size_t)1);
				delay = (1000000000) / (packetsPerSecond);
			}
			
			// every second we are writing reports
			if (now - lastReportAt > 1000) {
				RScopeLock lock(mutex);
				
//				std::cout << FORMAT_COL << bytesToDisplay(bytesPerSecond) + "/s";
				std::cout << FORMAT_COL << "byte sent";
				std::cout << FORMAT_COL << "pkts sent";
//				std::cout << FORMAT_COL << "delay";
				std::cout << FORMAT_COL << "scale";
				switch (type) {
					case PUB_RTP: {
						std::cout << FORMAT_COL << "[RTP]";
						break;
					}
					case PUB_MCAST: {
						std::cout << FORMAT_COL << "[RTP/MCAST]";
						break;
					}
					case PUB_TCP: {
						std::cout << FORMAT_COL << "[TCP]";
						break;
					}
				}
				std::cout << FORMAT_COL << std::endl;

				
				std::cout << FORMAT_COL << bytesToDisplay(bytesWritten) + "/s";
				std::cout << FORMAT_COL << toStr(packetsWritten) + "pkt/s";
//				std::cout << FORMAT_COL << timeToDisplay(delay);

				if (reports.size() == 0) {
					std::cout << FORMAT_COL << "---";
					std::cout << std::endl;
				} else {
					
					double decreasePressure = 0;
					
					// print report messages
					std::map<std::string, Report>::iterator repIter = reports.begin();

					if (fixedBytesPerSecond == 0) {
						while(repIter != reports.end()) {
							const Report& report = repIter->second;
							// see if we need to decrease bandwidth
							if (report.pktsLate > packetsWritten / 2) {
								// client is lagging more than half a second, scale back somewhat
								decreasePressure = (std::max)(1.1, decreasePressure);
							}
							if (report.pcntLoss > pcntLossOk) {
								// we lost packages, scale back somewhat
								decreasePressure = (std::max)((double)report.pktsDropped / report.pktsRcvd, decreasePressure);
								decreasePressure = (std::min)(decreasePressure, 4.0);
							}
							if (report.pktsLate > 3 * packetsWritten) {
								// queues explode! scale back alot!
								decreasePressure = (std::max)((double)report.pktsLate / packetsWritten, decreasePressure);
								decreasePressure = (std::min)(decreasePressure, 4.0);
							}
							repIter++;
						}
						
						if (decreasePressure > 0) {
							bytesPerSecond *= (1.0 / decreasePressure);
							bytesPerSecond = (std::max)(bytesPerSecond, (size_t)(1024));
							std::cout << FORMAT_COL << "down " + toStr(1.0 / decreasePressure);
						} else {
							bytesPerSecond *= 1.2;
							std::cout << FORMAT_COL << "up 1.2";
						}
					} else {
						if (bytesWritten < fixedBytesPerSecond) {
							bytesPerSecond *= 1.05;
						} else {
							bytesPerSecond *= 0.95;
						}
					}
					std::cout << FORMAT_COL << "---";
					std::cout << std::endl;
					
					std::cout << FORMAT_COL << "byte recv";
					std::cout << FORMAT_COL << "pkts recv";
					std::cout << FORMAT_COL << "pkts loss";
					std::cout << FORMAT_COL << "% loss";
					std::cout << FORMAT_COL << "pkts late";
					std::cout << FORMAT_COL << "latency";
					std::cout << FORMAT_COL << "name";
					std::cout << std::endl;
					
					repIter = reports.begin();
					while(repIter != reports.end()) {
						const Report& report = repIter->second;
						std::cout << FORMAT_COL << bytesToDisplay(report.bytesRcvd) + "/s";
						std::cout << FORMAT_COL << toStr(report.pktsRcvd) + "pkt/s";
						std::cout << FORMAT_COL << toStr(report.pktsDropped) + "pkt/s";
						std::cout << FORMAT_COL << toStr(report.pcntLoss) + "%";
						std::cout << FORMAT_COL << (report.pktsLate > 100000 ? "N/A" : toStr(report.pktsLate) + "pkt");
						std::cout << FORMAT_COL << (report.latency > 100000 ? "N/A" : toStr(report.latency) + "ms");
						if (repIter->first.size() >= 6) {
							std::cout << FORMAT_COL << repIter->first.substr(0, 6);
						} else {
							std::cout << FORMAT_COL << repIter->first.substr(0, 6);
						}
						std::cout << std::endl;

						repIter++;
					}

				}
				std::cout << std::endl;
				
				bytesWritten = 0;
				packetsWritten = 0;
				reports.clear();
				
				lastReportAt = Thread::getTimeStampMs();
			}
			Thread::sleepNs(delay);
		}
		delete(msg);

	} else {
		ThroughputReceiver tpRcvr;
		
		reporter = Publisher("reports");

		Subscriber tcpSub("throughput.tcp", &tpRcvr);
		
		RTPSubscriberConfig mcastConfig;
		mcastConfig.setMulticastIP("224.1.2.3");
		mcastConfig.setMulticastPortbase(42042);
		Subscriber mcastSub(Subscriber::RTP, "throughput.mcast", &tpRcvr, &mcastConfig);

		RTPSubscriberConfig rtpConfig;
		rtpConfig.setPortbase(40042);
		Subscriber rtpSub(Subscriber::RTP, "throughput.rtp", &tpRcvr, &rtpConfig);

		node.addSubscriber(tcpSub);
		node.addSubscriber(mcastSub);
		node.addSubscriber(rtpSub);
		node.addPublisher(reporter);
		
		// do nothing here, we reply only when we received a message
		while(true) {
			Thread::sleepMs(1000);
		}
	}
}
