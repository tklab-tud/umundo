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
#include <fstream>      // std::fstream

#ifdef WIN32
#include "XGetopt.h"
#endif

#ifdef UNIX
#include <unistd.h>
#endif

#define FORMAT_COL std::setw(12) << std::left

using namespace umundo;

enum PubType {
	PUB_RTP,
	PUB_TCP,
	PUB_MCAST
};

RMutex mutex;
bool isClient = false;
bool isServer = false;
uint64_t currSeqNr = 0;
uint32_t reportInterval = 1000;
uint64_t timeStampStartedAt = 0;
size_t currReportNr = 0;

Discovery disc;
Node node;

// server
size_t bytesPerSecond = 1024 * 1024;
size_t fixedBytesPerSecond = 0;
size_t mtu = 1280;
double pcntLossOk = 0;
size_t waitForSubs = 0;
size_t packetsWritten = 0;
PubType type = PUB_TCP;
bool useZeroCopy = false;
uint64_t bytesTotal = 0;
uint64_t bytesWritten = 0;
double intervalFactor = 1;
size_t delay = 0;
uint64_t duration = 0;
std::string filePrefix;

// client
size_t bytesRcvd = 0;
size_t pktsRecvd = 0;
size_t pktsDropped = 0;
size_t lastSeqNr = 0;
size_t discoveryTimeMs = 0;
uint64_t timeStampServerLast = 0;
uint64_t timeStampServerFirst = 0;
std::string serverUUID;
Publisher reporter;


class Report {
public:
	Report() : reportNr(0), pktsRcvd(0), pktsDropped(0), bytesRcvd(0), pktsLate(0), roundTripTime(0), discoveryTime(0), pertainingPacket(0), pcntLoss(0) {}
	size_t reportNr;
	size_t pktsRcvd;
	size_t pktsDropped;
	size_t bytesRcvd;
	size_t pktsLate;
	size_t roundTripTime;
	size_t discoveryTime;
	size_t pertainingPacket;
	double pcntLoss;
	std::string pubId;
	std::string hostId;
	std::string hostName;
	
	static void writeCSVHead(std::ostream& stream) {
		stream << "\"Discovery Time\", \"Bytes/s\", \"Packets/s\", \"Packets delayed\", \"RTT\", \"Loss %\", \"Packets Dropped\", \"Hostname\"" << std::endl;
	}
	
	void writeCSVData(std::ostream& stream) {
		stream << discoveryTime << ", ";
		stream << bytesRcvd << ", ";
		stream << pktsRcvd << ", ";
		stream << pktsLate << ", ";
		stream << roundTripTime << ", ";
		stream << pcntLoss << ", ";
		stream << pktsDropped << ", ";
		stream << hostName << std::endl;
	}
};

std::map<std::string, Report> reports;
std::map<std::string, std::list<Report> > allReports;
std::list<Report> senderReports;

class DiscoveryTimeGreeter : public Greeter {
public:
	DiscoveryTimeGreeter() {}
	void welcome(Publisher& pub, const SubscriberStub& sub) {
		discoveryTimeMs = Thread::getTimeStampMs() - timeStampStartedAt;
	}
	void farewell(Publisher&, const SubscriberStub&) {}
};

class ThroughputReceiver : public Receiver {
	void receive(Message* msg) {

		RScopeLock lock(mutex);

		uint64_t currServerTimeStamp;
		Message::read(msg->data(), &currSeqNr);
		Message::read(msg->data() + 8, &currServerTimeStamp);
		Message::read(msg->data() + 16, &reportInterval);

		if (currSeqNr < lastSeqNr) {
			// new throughput run!
			lastSeqNr = 0;
			timeStampServerFirst = 0;
			currReportNr = 0;
			bytesRcvd = 0;
			pktsRecvd = 0;
			pktsDropped = 0;
		}

		bytesRcvd += msg->size();
		pktsRecvd++;

		if (timeStampServerFirst == 0)
			timeStampServerFirst = currServerTimeStamp;

		if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
			pktsDropped += currSeqNr - lastSeqNr;
		}

		lastSeqNr = currSeqNr;

		// reply?
		if (currServerTimeStamp - reportInterval >= timeStampServerLast) {
			RScopeLock lock(mutex);
			timeStampServerLast = currServerTimeStamp;

			Message* msg = new Message();
			msg->putMeta("bytes.rcvd", toStr(bytesRcvd));
			msg->putMeta("pkts.dropped", toStr(pktsDropped));
			msg->putMeta("pkts.rcvd", toStr(pktsRecvd));
			msg->putMeta("last.seq", toStr(lastSeqNr));
			msg->putMeta("report.seq", toStr(currReportNr++));
			msg->putMeta("timestamp.server.last", toStr(timeStampServerLast));
			msg->putMeta("timestamp.server.first", toStr(timeStampServerFirst));
			msg->putMeta("hostname", umundo::Host::getHostname());
			reporter.send(msg);
			delete msg;

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

		std::cout << ".";
		
		std::string pubUUID = msg->getMeta("um.pub");
		Report& report = reports[pubUUID];

		report.reportNr      = strTo<size_t>(msg->getMeta("report.seq"));
		report.pktsRcvd      = strTo<size_t>(msg->getMeta("pkts.rcvd"));
		report.pktsDropped   = strTo<size_t>(msg->getMeta("pkts.dropped"));
		report.bytesRcvd     = strTo<size_t>(msg->getMeta("bytes.rcvd"));
		report.hostName      = msg->getMeta("hostname");
		report.hostId        = msg->getMeta("um.host");
		report.pubId         = msg->getMeta("um.pub");
		
		report.roundTripTime = Thread::getTimeStampMs() - strTo<size_t>(msg->getMeta("timestamp.server.last"));
		report.discoveryTime = strTo<uint64_t>(msg->getMeta("timestamp.server.first")) - timeStampStartedAt;

		size_t lastSeqNr        = strTo<size_t>(msg->getMeta("last.seq"));
		report.pktsLate         = currSeqNr - lastSeqNr;
		report.pertainingPacket = lastSeqNr;

		if (report.pktsDropped > 0) {
			double onePerc = (double)(report.pktsDropped + report.pktsRcvd) * 0.01;
			onePerc = (std::max)(onePerc, 0.0001);
			report.pcntLoss = (std::min)((double)report.pktsDropped / onePerc, 100.0);
		} else {
			report.pcntLoss = 0;
		}
		
#if 0
		if (allReports.find(report.pubId) == allReports.end()) {
			for (int i = 0; i < currReportNr; i++) // add empty reports for padding
				allReports[report.pubId].push_back(Report());
		}
#endif

		if (allReports.find(report.pubId) != allReports.end() &&
				allReports[report.pubId].back().pertainingPacket > report.pertainingPacket) {
			
			std::cerr << "Received report out of order: " << allReports[report.pubId].back().pertainingPacket << ">" << report.pertainingPacket << " - ignoring";
		} else {
			allReports[report.pubId].push_back(report);
		}
	}
};

class ThroughputGreeter : public Greeter {
	void welcome(Publisher&, const SubscriberStub&) {
	}

	void farewell(Publisher& pub, const SubscriberStub& subStub) {
//		RScopeLock lock(mutex);
//		if (reports.find(subStub.getUUID()) != reports.end())
//			reports.erase(subStub.getUUID());
	}
};

void printUsageAndExit() {
	printf("umundo-throughput version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-throughput -s|-c [-l N] [-m N] [-w N] [-d N] [-oPREFIX]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c                 : act as a client\n");
	printf("\t-s                 : act as a server\n");
	printf("\t-f BYTES/s         : try to maintain given throughput\n");
	printf("\t-t [rtp|tcp|mcast] : type of publisher to measure throughput\n");
	printf("\t-d                 : duration in second to keep server running\n");
	printf("\t-o PREFIX          : after duration elapsed, write report files with given name prefix\n");
	printf("\t-z                 : employ zero-copy (experimental)\n");
	printf("\t-i                 : report interval in milli-seconds\n");
	printf("\t-l                 : acceptable packet loss in percent\n");
	printf("\t-m <number>        : MTU to use on server (defaults to 1280)\n");
	printf("\t-w <number>        : wait for given number of subscribers before testing\n");
	exit(1);
}

std::string bytesToDisplay(uint64_t nrBytes) {
	std::stringstream ss;
	ss << std::setprecision(5);

	uint64_t kMax = (uint64_t)1024 * (uint64_t)1024;
	uint64_t mMax = kMax * (uint64_t)1024;
	uint64_t gMax = mMax * (uint64_t)1024;

	if (nrBytes < 1024) {
		ss << nrBytes << "B";
		return ss.str();
	}
	if (nrBytes < kMax) {
		ss << ((double)nrBytes / (double)1024) << "KB";
		return ss.str();
	}
	if (nrBytes < mMax) {
		ss << ((double)nrBytes / (double)kMax) << "MB";
		return ss.str();
	}
	if (nrBytes < gMax) {
		ss << ((double)nrBytes / (double)mMax) << "GB";
		return ss.str();
	}
	ss << ((double)nrBytes / (double)gMax) << "TB";
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

void doneCallback(void* data, void* hint) {
}

void runAsClient() {
	ThroughputReceiver tpRcvr;
	DiscoveryTimeGreeter discGreeter;
	
	reporter = Publisher("reports");
	reporter.setGreeter(&discGreeter);
	
	Subscriber tcpSub("throughput.tcp");
	tcpSub.setReceiver(&tpRcvr);
	
	SubscriberConfigMCast mcastConfig("throughput.mcast");
	mcastConfig.setMulticastIP("224.1.2.3");
	mcastConfig.setMulticastPortbase(22022);
	Subscriber mcastSub(&mcastConfig);
	mcastSub.setReceiver(&tpRcvr);
	
	SubscriberConfigRTP rtpConfig("throughput.rtp");
	//		rtpConfig.setPortbase(40042);
	Subscriber rtpSub(&rtpConfig);
	rtpSub.setReceiver(&tpRcvr);
	
	node.addSubscriber(tcpSub);
	node.addSubscriber(mcastSub);
	node.addSubscriber(rtpSub);
	node.addPublisher(reporter);
	
	disc.add(node);
	timeStampStartedAt = Thread::getTimeStampMs();
	
	// do nothing here, we reply only when we received a message
	while(true) {
#if 0
		Thread::sleepMs(1000);
#else
		// disconnect / reconnect client on key-press
		getchar();
		node.removeSubscriber(tcpSub);
		node.removeSubscriber(mcastSub);
		node.removeSubscriber(rtpSub);
		node.removePublisher(reporter);
		disc.remove(node);
		
		getchar();
		disc.add(node);
		node.addSubscriber(tcpSub);
		node.addSubscriber(mcastSub);
		node.addSubscriber(rtpSub);
		node.addPublisher(reporter);
		
#endif
	}
}

void writeReports(std::string scale) {
	std::map<std::string, Report>::iterator repIter = reports.begin();

	std::cout << std::endl;
	std::cout << FORMAT_COL << "byte sent";
	std::cout << FORMAT_COL << "pkts sent";
	std::cout << FORMAT_COL << "sleep";
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
	std::cout << FORMAT_COL << bytesToDisplay(bytesTotal);
	std::cout << FORMAT_COL << std::endl;
	// ===
	
	std::cout << FORMAT_COL << bytesToDisplay(intervalFactor * (double)bytesWritten) + "/s";
	std::cout << FORMAT_COL << toStr(intervalFactor * (double)packetsWritten) + "pkt/s";
	std::cout << FORMAT_COL << timeToDisplay(delay * 1000);
	std::cout << FORMAT_COL << scale;

	std::cout << FORMAT_COL << "---";
	std::cout << std::endl;

	if (reports.size() > 0) {

		// print report messages
		
		std::cout << FORMAT_COL << "byte recv";
		std::cout << FORMAT_COL << "pkts recv";
		std::cout << FORMAT_COL << "pkts loss";
		std::cout << FORMAT_COL << "% loss";
		std::cout << FORMAT_COL << "pkts late";
		std::cout << FORMAT_COL << "RTT";
		std::cout << FORMAT_COL << "dscvd";
		std::cout << FORMAT_COL << "name";
		std::cout << std::endl;
		
		repIter = reports.begin();
		while(repIter != reports.end()) {
			Report& report = repIter->second;
			std::cout << FORMAT_COL << bytesToDisplay(intervalFactor * (double)(report.bytesRcvd)) + "/s";
			std::cout << FORMAT_COL << toStr(intervalFactor * (double)(report.pktsRcvd)) + "pkt/s";
			std::cout << FORMAT_COL << toStr(intervalFactor * (double)(report.pktsDropped)) + "pkt/s";
			std::cout << FORMAT_COL << toStr(report.pcntLoss) + "%";
			std::cout << FORMAT_COL << (report.pktsLate > 100000 ? "N/A" : toStr(report.pktsLate) + "pkt");
			std::cout << FORMAT_COL << (report.roundTripTime > 100000 ? "N/A" : toStr(report.roundTripTime) + "ms");
			std::cout << FORMAT_COL << (report.discoveryTime > 100000 ? "N/A" : toStr(report.discoveryTime) + "ms");
			std::cout << FORMAT_COL << report.pubId.substr(0, 6) + " (" + report.hostName + ")";
			std::cout << std::endl;
			
			report.pktsRcvd = 0;
			report.bytesRcvd = 0;
			report.pktsLate = 0;
			report.roundTripTime = 10000000;
			repIter++;
		}
	}
	//				reports.clear();

	// append our information into allReports
	Report senderReport;
	senderReport.bytesRcvd = bytesWritten;
	senderReport.pktsRcvd = packetsWritten;
	senderReport.pertainingPacket = currSeqNr;
	senderReports.push_back(senderReport);
}

void runAsServer() {
	ThroughputGreeter tpGreeter;
	
	Publisher pub;
	switch (type) {
		case PUB_RTP: {
			PublisherConfigRTP config("throughput.rtp");
			config.setTimestampIncrement(166);
			pub = Publisher(&config);
			pub.setGreeter(&tpGreeter);
			break;
		}
		case PUB_MCAST: {
			PublisherConfigMCast config("throughput.mcast");
			config.setTimestampIncrement(166);
			//			config.setPortbase(42142);
			pub = Publisher(&config);
			pub.setGreeter(&tpGreeter);
			break;
		}
		case PUB_TCP: {
			pub = Publisher("throughput.tcp");
			pub.setGreeter(&tpGreeter);
			break;
		}
	}
	
	node.addPublisher(pub);
	
	disc.add(node);
	timeStampStartedAt = Thread::getTimeStampMs();
	
	if (duration > 0)
		duration *= 1000; // convert to ms
	
	pub.waitForSubscribers(waitForSubs);
	
	// reserve 20 bytes for timestamp, sequence number and report interval
	size_t dataSize = (std::max)(mtu, (size_t)20);
	char* data = (char*)malloc(dataSize);
	Message* msg = NULL;
	if (useZeroCopy) {
		msg = new Message(data, dataSize, doneCallback, (void*)NULL);
	} else {
		msg = new Message();
	}
	
	uint64_t lastReportAt = Thread::getTimeStampMs();

	while(1) {
		uint64_t now = Thread::getTimeStampMs();
		
		if (duration > 0 && now - timeStampStartedAt > duration)
			break;
		
		// first 16 bytes are seqNr and timestamp
		Message::write(&data[0], ++currSeqNr);
		Message::write(&data[8], now);
		Message::write(&data[16], reportInterval);
		
		if (!useZeroCopy) {
			msg->setData(data, dataSize);
		}
		
		pub.send(msg);
		
		intervalFactor = 1000.0 / (double)reportInterval;
		bytesWritten += dataSize;
		bytesTotal += dataSize;
		packetsWritten++;
		
		// sleep just enough to reach the desired bps
		{
			RScopeLock lock(mutex);
			size_t packetsPerSecond = (std::max)(bytesPerSecond / dataSize, (size_t)1);
			delay = (std::max)((1000000) / (packetsPerSecond), (size_t)1);
		}
		
		// every report interval we are recalculating bandwith
		if (now - lastReportAt > reportInterval) {
			RScopeLock lock(mutex);
			
			std::string scaleInfo("--");
			// and recalculate bytes to send
			if (reports.size() > 0) {
				double decreasePressure = 0;
				
				// print report messages
				std::map<std::string, Report>::iterator repIter = reports.begin();
				
				// bandwidth is not fixed
				if (fixedBytesPerSecond == 0) {
					while(repIter != reports.end()) {
						const Report& report = repIter->second;
						double reportPressure = 0;
						
						// see if we need to decrease bandwidth
						if (report.pktsLate > packetsWritten / 2) {
							// client is lagging more than half a second, scale back somewhat
							reportPressure = (std::max)(1.1, reportPressure);
						}
						if (report.pcntLoss > pcntLossOk) {
							// we lost packages, scale back somewhat
							reportPressure = (std::max)((double)report.pktsDropped / report.pktsRcvd, reportPressure);
							reportPressure = (std::min)(reportPressure, 1.4);
						}
						if (report.pktsLate > 3 * packetsWritten) {
							// queues explode! scale back alot!
							reportPressure = (std::max)((double)report.pktsLate / packetsWritten, reportPressure);
							reportPressure = (std::min)(reportPressure, 2.0);
						}
						
						if (reportPressure > decreasePressure)
							decreasePressure = reportPressure;
						
						repIter++;
					}
					
					if (decreasePressure > 1) {
						bytesPerSecond *= (1.0 / decreasePressure);
						bytesPerSecond = (std::max)(bytesPerSecond, (size_t)(1024));
						scaleInfo = "down " + toStr(1.0 / decreasePressure);
					} else {
						bytesPerSecond *= 1.2;
						scaleInfo = "up 1.2";
					}
					std::cout << std::endl;
				} else {
					
					if (bytesWritten * intervalFactor < fixedBytesPerSecond) {
						bytesPerSecond *= 1.05;
					} else if (bytesWritten * intervalFactor > fixedBytesPerSecond)  {
						bytesPerSecond *= 0.95;
					}
				}
			}
			currReportNr++;

			writeReports(scaleInfo);

			bytesWritten = 0;
			packetsWritten = 0;
			
			lastReportAt = Thread::getTimeStampMs();
		}

		// now we sleep until we have to send another packet
		Thread::sleepUs(delay);
	}
	
	pub.setGreeter(NULL);
	delete(msg);
}

int main(int argc, char** argv) {
	int option;
	while ((option = getopt(argc, argv, "zcsm:w:l:f:t:i:o:d:")) != -1) {
		switch(option) {
		case 'z':
			useZeroCopy = true;
			break;
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
		case 'd':
			duration = atoi((const char*)optarg);
			break;
		case 'o':
			filePrefix = optarg;
			break;
		case 'w':
			waitForSubs = atoi((const char*)optarg);
			break;
		case 'i':
			reportInterval = atoi((const char*)optarg);
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

	DiscoveryConfigMDNS discConfig;
//	discConfig.setDomain("throughput");
	disc = Discovery(&discConfig);
	
	Subscriber sub("reports");
	ReportReceiver reportRecv;
	sub.setReceiver(&reportRecv);
	node.addSubscriber(sub);

	if (isServer) {
		runAsServer();
	} else {
		runAsClient();
	}
	
	if (filePrefix.size() > 0) {
		// cannot have ofstream in a map (non-copyable), works with LLVM though
#ifdef APPLE
		// wait for missing reports, then make sure no more reports are coming in
		Thread::sleepMs(60000);
		sub.setReceiver(NULL);
		
		// write files
		std::map<std::string, std::list<Report> >::iterator reportIter = allReports.begin();
		std::map<std::string, std::ofstream> fileHandles;
		
		// open a file per reporter and write header
		while(reportIter != allReports.end()) {
			std::string pubUUID = reportIter->first;
			std::string reportFileName = filePrefix + pubUUID.substr(0,8) + ".data";
			fileHandles[pubUUID].open(reportFileName.c_str(), std::fstream::out);
			Report::writeCSVHead(fileHandles[pubUUID]);
#ifdef UNIX
			// that's only useful if there is only a single reporter per host
			std::string linkFrom = filePrefix + pubUUID.substr(0,8) + ".data";
			std::string linkTo = filePrefix + reportIter->second.back().hostId + ".host.data";
			
			unlink(linkTo.c_str());
			symlink(linkFrom.c_str(), linkTo.c_str());
#endif
			reportIter++;
		}
		
		// write reports
		std::fstream senderReport;
		std::string senderReportFileName = filePrefix + "sender.data";
		senderReport.open(senderReportFileName.c_str(), std::fstream::out);
		senderReport << "\"Bytes Sent\", \"Packets sent\"" << std::endl;

		std::list<Report>::iterator senderRepIter = senderReports.begin();
		while(senderRepIter!= senderReports.end()) {
			senderReport << senderRepIter->bytesRcvd << ", ";
			senderReport << senderRepIter->pktsRcvd << std::endl;
			senderRepIter++;
			
			reportIter = allReports.begin();
			while(reportIter != allReports.end()) {

				if (reportIter->second.size() == 0) {
					// all data consumed - insert missing at end
					fileHandles[reportIter->first] << "-" << std::endl;
					break;
				}
				
				if (reportIter->second.front().pertainingPacket > senderRepIter->pertainingPacket) {
					// no data for reporting period!
					fileHandles[reportIter->first] << "-" << std::endl;
				} else {
					reportIter->second.front().writeCSVData(fileHandles[reportIter->first]);
					reportIter->second.pop_front();
				}
				
#if 0
				// discard multiple reports?
				while(reportIter->second.size() > 0 &&
							reportIter->second.front().pertainingPacket < senderRepIter->pertainingPacket) {
					reportIter->second.pop_front();
				}
#endif
				reportIter++;
			}
		}

		std::map<std::string, std::ofstream>::iterator fhIter = fileHandles.begin();
		while(fhIter != fileHandles.end()) {
			fhIter->second.close();
			fhIter++;
		}

		senderReport.close();

#if 0
		while(true) {
			reportIter = allReports.begin();
			size_t nextReportingEnd = 4294967295; // 2**32 - 1
			std::string nextReporter;

			// break when we consumed all reports
			if (allReports.size() == 0)
				break;

			while(reportIter != allReports.end()) {
				// walk through the report iterators and find next report due
				size_t nextDueReport = reportIter->second.front().pertainingPacket;
				if (nextDueReport < nextReportingEnd) {
					nextReportingEnd = nextDueReport;
					nextReporter     = reportIter->first;
				}
				reportIter++;
			}
			// ok, write earliest report into earlist's reports file
			Report& nextReport = allReports[nextReporter].front();
			
			fileHandles[nextReporter] << nextReport.discoveryTime << ", ";
			fileHandles[nextReporter] << nextReport.bytesRcvd << ", ";
			fileHandles[nextReporter] << nextReport.pktsRcvd << ", ";
			fileHandles[nextReporter] << nextReport.pktsLate << ", ";
			fileHandles[nextReporter] << nextReport.roundTripTime << ", ";
			fileHandles[nextReporter] << nextReport.pcntLoss << ", ";
			fileHandles[nextReporter] << nextReport.pktsDropped << ", ";
			fileHandles[nextReporter] << nextReport.hostName << std::endl;

			allReports[nextReporter].pop_front();
			
			if (allReports[nextReporter].size() == 0) {
				allReports.erase(nextReporter);
			} else {
				// fill with newlines if we skipped reports
				Report& upcomingReport = allReports[nextReporter].front();
				for(int i = 1; nextReport.reportNr + i < upcomingReport.reportNr; i++) {
					fileHandles[nextReporter] << std::endl;
				}
			}
		}
		
		std::map<std::string, std::fstream>::iterator fhIter = fileHandles.begin();
		while(fhIter != fileHandles.end()) {
			fhIter->second.close();
			fhIter++;
		}
		
		// write sender reports
		std::fstream senderReport;
		std::string reportFileName = filePrefix + "sender.data";
		senderReport.open(reportFileName.c_str(), std::fstream::out);
		std::list<Report>::iterator senderRepIter = senderReports.begin();
		while(senderRepIter!= senderReports.end()) {
			senderReport << senderRepIter->bytesRcvd << ", ";
			senderReport << senderRepIter->pktsRcvd << std::endl;
			senderRepIter++;
		}
		senderReport.close();
#endif
#endif
	}
}
