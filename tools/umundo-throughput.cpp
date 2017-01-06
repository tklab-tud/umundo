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
#include "umundo/util/crypto/MD5.h"

#include <iostream>
#include <iomanip>
#include <string.h>
#include <algorithm>    // std::max
#include <fstream>      // std::fstream

#ifdef WIN32
#include <windows.h>
#include "XGetopt.h"
#endif

#ifdef UNIX
#include <unistd.h>
#endif

extern "C" {
#include "datagen.h"
}

#define FORMAT_COL std::setw(14) << std::left

#define MIN_MTU 20

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
size_t currReportNr = 0;

Discovery disc;
Node node;

// server
size_t bytesPerSecond = 1024 * 1024;
size_t fixedBytesPerSecond = 0;
uint64_t timeStampStartedDiscovery = 0;
uint64_t timeStampStartedPublishing = 0;
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
uint64_t waitToConclude = 2000;
std::string filePrefix;
char* streamFile = NULL;
std::string streamData;

std::string compressionType = "";
int compressionLevel = -1;
bool compressionWithState = false;
int compressionRefreshInterval = 0;
double compressionActualRatio = 100;

// client
size_t bytesRcvd = 0;
size_t pktsRecvd = 0;
size_t pktsDropped = 0;
double compressRatioHead = 0;
double compressRatioPayload = 0;
double compressionRollOff = 0;
size_t lastSeqNr = 0;
size_t discoveryTimeMs = 0;
uint64_t timeStampServerLast = 0;
uint64_t timeStampServerFirst = 0;
std::string serverUUID;
Publisher reporter;


class Report {
public:
	Report() : reportNr(0), pktsRcvd(0), pktsDropped(0), bytesRcvd(0), pktsLate(0), roundTripTime(0), discoveryTime(0), pertainingPacket(0), pcntLoss(0), compressRatioHead(100), compressRatioPayload(100) {}
    size_t reportNr;
	size_t pktsRcvd;
	size_t pktsDropped;
	size_t bytesRcvd;
	size_t pktsLate;
	size_t roundTripTime;
	size_t discoveryTime;
	size_t pertainingPacket;
	double pcntLoss;
    double compressRatioHead;
    double compressRatioPayload;
	std::string pubId;
	std::string hostId;
	std::string hostName;

	static void writeCSVHead(std::ostream& stream) {
		stream << "\"Timestamp\", \"Discovery Time\", \"Bytes\", \"Packets\", \"Packets delayed\", \"RTT\", \"Loss %\", \"Ratio Head\", \"Ratio Payload\", \"Packets Dropped\", \"Hostname\"" << std::endl;
	}

	void writeCSVData(std::ostream& stream) {
        stream << reportNr * reportInterval << ", ";
        stream << discoveryTime << ", ";
		stream << bytesRcvd << ", ";
		stream << pktsRcvd << ", ";
		stream << pktsLate << ", ";
		stream << roundTripTime << ", ";
		stream << pcntLoss << ", ";
        stream << compressRatioHead << ", ";
        stream << compressRatioPayload << ", ";
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
		discoveryTimeMs = Thread::getTimeStampMs() - timeStampStartedDiscovery;
	}
	void farewell(Publisher&, const SubscriberStub&) {}
};

class ThroughputReceiver : public Receiver {
	void receive(Message* msg) {

		RScopeLock lock(mutex);

        if (msg->getMeta("md5").size() > 0 && msg->getMeta("md5") != md5(msg->data(), msg->size())) {
            UM_LOG_WARN("Corrupted message received");
        }
            
        
		uint64_t currServerTimeStamp;
		Message::read(msg->data(), &currSeqNr);
		Message::read(msg->data() + 8, &currServerTimeStamp);
		Message::read(msg->data() + 16, &reportInterval);

		if (currSeqNr < lastSeqNr) {
			// new throughput run!
			lastSeqNr = 0;
			timeStampServerFirst = 0;
            timeStampServerLast = 0;
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

        compressRatioHead    += (msg->getMeta().find("um.compressRatio.head") != msg->getMeta().end() ? strTo<double>(msg->getMeta("um.compressRatio.head")) : 100);
        compressRatioPayload += (msg->getMeta().find("um.compressRatio.payload") != msg->getMeta().end() ? strTo<double>(msg->getMeta("um.compressRatio.payload")) : 100);
        
//        if (timeStampServerLast == 0)
//            timeStampServerLast = currServerTimeStamp;

        // may happen when initializing timeStampServerLast in client
        if (timeStampServerLast > currServerTimeStamp)
            timeStampServerLast = currServerTimeStamp;
        
		// reply?
		if (currServerTimeStamp - reportInterval >= timeStampServerLast) {
			RScopeLock lock(mutex);
			timeStampServerLast = currServerTimeStamp;
            
			Message* msg = new Message();
			msg->putMeta("bytes.rcvd", toStr(bytesRcvd));
			msg->putMeta("pkts.dropped", toStr(pktsDropped));
            msg->putMeta("compress.ratio.head", toStr(compressRatioHead / (double)pktsRecvd));
            msg->putMeta("compress.ratio.payload", toStr(compressRatioPayload / (double)pktsRecvd));
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
            compressRatioHead = 0;
            compressRatioPayload = 0;
            
		}
        
	}
};

class ReportReceiver : public Receiver {
public:
	ReportReceiver() {};
	void receive(Message* msg) {
		RScopeLock lock(mutex);

//		std::cout << ".";

		std::string pubUUID = msg->getMeta("um.pub");
		Report& report = reports[pubUUID];

		report.reportNr      = strTo<size_t>(msg->getMeta("report.seq"));
		report.pktsRcvd      = strTo<size_t>(msg->getMeta("pkts.rcvd"));
		report.pktsDropped   = strTo<size_t>(msg->getMeta("pkts.dropped"));
		report.bytesRcvd     = strTo<size_t>(msg->getMeta("bytes.rcvd"));
        if (compressionType.size() != 0) {
            report.compressRatioHead = strTo<double>(msg->getMeta("compress.ratio.head"));
            report.compressRatioPayload = strTo<double>(msg->getMeta("compress.ratio.payload"));
        }
		report.hostName      = msg->getMeta("hostname");
		report.hostId        = msg->getMeta("um.host");
		report.pubId         = msg->getMeta("um.pub");

		report.roundTripTime = Thread::getTimeStampMs() - strTo<size_t>(msg->getMeta("timestamp.server.last"));
		report.discoveryTime = strTo<uint64_t>(msg->getMeta("timestamp.server.first")) - timeStampStartedDiscovery;

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
	printf("umundo-throughput version " UMUNDO_VERSION " (" UMUNDO_PLATFORM_ID " " CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-throughput -s|-c [-r BYTES/s] [-l N] [-m N] [-w N] [-d N] [-e N]\n");
	printf("\t                  [-(x,y) fastlz|miniz|lz4[:level[:refresh]]] [-f (FILE|size:comp)] [-o PREFIX]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c                  : act as a client\n");
	printf("\t-s                  : act as a server\n");
	printf("\t-r BYTES/s          : try to maintain given throughput rate\n");
	printf("\t-t [rtp|tcp|mcast]  : type of publisher to measure throughput\n");
	printf("\t-d                  : duration in second to keep server running\n");
	printf("\t-o PREFIX           : after duration elapsed, write report files\n");
	printf("\t-z                  : employ zero-copy (experimental)\n");
	printf("\t-i                  : report interval in milli-seconds\n");
	printf("\t-l                  : acceptable packet loss in percent\n");
    printf("\t-x ALG:LVL          : use compression algorithm (fastlz|miniz|lz4)\n");
    printf("\t-y ALG:LVL:RFRSH    : use compression with state and refresh at intervals given in ms\n");
    printf("\t-f (FILE|size:comp) : stream data from file or use synthetic data with given compressibility\n");
	printf("\t-m <number>         : MTU to use on server (defaults to 1280)\n");
	printf("\t-w <number>         : wait for given number of subscribers\n");
	printf("\t-e <number>         : after duration elapsed wait for pending reports\n");
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
    timeStampServerLast = Thread::getTimeStampMs();

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
	timeStampStartedPublishing = Thread::getTimeStampMs();

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
		std::cout << FORMAT_COL << "compression";
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

            // 0.0746/0.0227 inter message
            // 0.925/0.402 intra message
            std::stringstream comprSS;
            comprSS << std::setprecision(3) << report.compressRatioHead << "/" << std::setprecision(3) << report.compressRatioPayload;
            std::cout << FORMAT_COL << comprSS.str();

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
	PublisherConfig* config = NULL;

	switch (type) {
	case PUB_RTP: {
		config = new PublisherConfigRTP("throughput.rtp");
		((PublisherConfigRTP*)config)->setTimestampIncrement(166);
		break;
	}
	case PUB_MCAST: {
		config = new PublisherConfigMCast("throughput.mcast");
		((PublisherConfigMCast*)config)->setTimestampIncrement(166);
		//			config.setPortbase(42142);
		break;
	}
	case PUB_TCP: {
		config = new PublisherConfigTCP ("throughput.tcp");
		break;
	}
	}
	if (compressionType.size() != 0)
		config->enableCompression(compressionType, compressionWithState, compressionLevel, compressionRefreshInterval);

	pub = Publisher(config);
	pub.setGreeter(&tpGreeter);
	delete config;

	node.addPublisher(pub);

	disc.add(node);
    timeStampStartedDiscovery = Thread::getTimeStampMs();

	if (duration > 0)
		duration *= 1000; // convert to ms

    // open file with data
    std::ifstream fin(streamFile, std::ios::in | std::ios::binary);
    if (fin) {
        // streamFile exists on filesyste,, slurp content into streamData
        streamData = std::string((std::istreambuf_iterator<char>(fin) ),
                                 (std::istreambuf_iterator<char>()     ));
    } else {
        // stream file is no actual file, interpret as "size:compressability"
        char* compArg = streamFile;
        
        // http://stackoverflow.com/a/53878/990120
        std::list<std::string> compArgs;
        do {
            const char *begin = compArg;
            while(*compArg != ':' && *compArg)
                compArg++;
            compArgs.push_back(std::string(begin, compArg - begin));
        } while(0 != *compArg++);

        size_t streamDataSize = 0;
        double compressibility = 50;

        if (compArgs.size() < 1)
            printUsageAndExit();
        
        streamDataSize = displayToBytes(compArgs.front());
        compArgs.pop_front();
        
        if (!compArgs.empty()) {
            compressibility = strTo<double>(compArgs.front());
            if (toStr(compressibility) != compArgs.front())
                printUsageAndExit();
            compArgs.pop_front();
        }
        
        if (!compArgs.empty()) {
            printUsageAndExit();
        }
        
        streamData.resize(streamDataSize);
        RDG_genBuffer(&streamData[0], streamDataSize, 1 - (compressibility/(double)100), 0.0, 0);
    }
    
    {
        // determine compressionActualRatio
        Message msg(streamData.data(), streamData.size());
        size_t compressMaxPayload = msg.getCompressBounds("lz4", NULL, Message::Compression::PAYLOAD);
        char* compressPayloadData = (char*)malloc(compressMaxPayload);
        
        size_t compressActualPayload = msg.compress("lz4", NULL, compressPayloadData, compressMaxPayload, Message::Compression::PAYLOAD);
        compressionActualRatio = 100 * ((double)compressActualPayload / streamData.size());
    }
    
    pub.waitForSubscribers(waitForSubs);
    timeStampStartedPublishing = Thread::getTimeStampMs();

    uint64_t lastReportAt = Thread::getTimeStampMs();
    size_t streamDataOffset = 0;

	while(1) {
        
        // fill data from stream data
        Message* msg = new Message((char*)malloc(mtu), mtu);

        size_t msgDataOffset = 0;
        while(true) {
            size_t toRead = (mtu - msgDataOffset <= streamData.size() - streamDataOffset ? mtu - msgDataOffset : streamData.size() - streamDataOffset);
            memcpy(&msg->data()[msgDataOffset], &streamData[streamDataOffset], toRead);
            msgDataOffset += toRead;
            streamDataOffset += toRead;
//            UM_LOG_WARN("%d: %d / %d", streamData.size(), msgDataOffset, streamDataOffset);
            if (msgDataOffset == mtu)
                break;
            if (streamData.size() == streamDataOffset)
                streamDataOffset = 0;
        }
//        UM_LOG_WARN("%d: %s", msg->size(), md5(msg->data(), msg->size()).c_str());
        
		uint64_t now = Thread::getTimeStampMs();
		if (duration > 0 && now - timeStampStartedPublishing > duration)
			break;

		// first 16 bytes are seqNr and timestamp
		Message::write(&msg->data()[0], ++currSeqNr);
		Message::write(&msg->data()[8], now);
		Message::write(&msg->data()[16], reportInterval);

//        msg->putMeta("md5", md5(msg->data(), msg->size()));
		intervalFactor = 1000.0 / (double)reportInterval;
		bytesWritten += msg->size();
		bytesTotal += msg->size();
		packetsWritten++;

		// sleep just enough to reach the desired bps
		{
			RScopeLock lock(mutex);
			size_t packetsPerSecond = (std::max)(bytesPerSecond / msg->size(), (size_t)1);
			delay = (std::max)((1000000) / (packetsPerSecond), (size_t)1);
		}

		// sending with compression will alter msg->size(); not anymore ...
		pub.send(msg);

		// every report interval we are recalculating bandwith
		if (now - lastReportAt > reportInterval) {
			RScopeLock lock(mutex);

			std::string scaleInfo("--");
			// and recalculate bytes to send
			if (reports.size() > 0) {
				double decreasePressure = 0;

				// print report messages
				std::map<std::string, Report>::iterator repIter = reports.begin();

				// bandwidth is fixed
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
						bytesPerSecond *= 1.3;
						scaleInfo = "up 1.3";
					}
					std::cout << std::endl;
				} else if (fixedBytesPerSecond == (size_t)-1) {
					// do nothing
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

        delete(msg);

		// now we sleep until we have to send another packet
		if (delay > 50 && fixedBytesPerSecond != (size_t) - 1)
			Thread::sleepUs(delay);
	}

	pub.setGreeter(NULL);
}

int main(int argc, char** argv) {
	int option;
    streamFile = argv[0]; // default
	while ((option = getopt(argc, argv, "zcsm:w:l:r:f:t:i:o:d:x:y:e:")) != -1) {
		switch(option) {
		case 'z':
			useZeroCopy = true;
			break;
        case 'y':
                compressionWithState = true;
                // fall through
		case 'x': {
            char* compArg = optarg;
            
            // http://stackoverflow.com/a/53878/990120
            std::list<std::string> compArgs;
            do {
                const char *begin = compArg;
                while(*compArg != ':' && *compArg)
                    compArg++;

                compArgs.push_back(std::string(begin, compArg - begin));
                
            } while(0 != *compArg++);
            
            if (!compArgs.empty()) {
                compressionType = compArgs.front();
                compArgs.pop_front();
            } else {
                printUsageAndExit();
            }

            if (!compArgs.empty()) {
                compressionLevel = strTo<int>(compArgs.front());
                if (toStr(compressionLevel) != compArgs.front())
                    printUsageAndExit();
                compArgs.pop_front();
            }

            if (!compArgs.empty()) {
                if (!compressionWithState)
                    printUsageAndExit();
                compressionRefreshInterval = strTo<int>(compArgs.front());
                if (toStr(compressionRefreshInterval) != compArgs.front())
                    printUsageAndExit();
                compArgs.pop_front();
            }

		}
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
		case 'f':
			streamFile = optarg;
			break;
		case 'm':
			mtu = displayToBytes(optarg);
			break;
		case 'e':
			waitToConclude = strTo<uint64_t>(optarg);
			break;
		case 'r':
			if (strcmp(optarg, "max") == 0 || strcmp(optarg, "inf") == 0) {
				fixedBytesPerSecond = (size_t)-1;
			} else {
				fixedBytesPerSecond = displayToBytes(optarg);
			}
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

	if (mtu < MIN_MTU)
		mtu = MIN_MTU;

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
		// wait for missing reports, then make sure no more reports are coming in
		Thread::sleepMs(waitToConclude);
		sub.setReceiver(NULL);

		// write files
		std::map<std::string, std::list<Report> >::iterator reportIter = allReports.begin();
		std::map<std::string, std::ofstream*> fileHandles;

		// open a file per reporter and write header
		while(reportIter != allReports.end()) {
			std::string pubUUID = reportIter->first;
			std::string reportFileName = filePrefix + pubUUID.substr(0,8) + ".data";
			fileHandles[pubUUID] = new std::ofstream();
			fileHandles[pubUUID]->open(reportFileName.c_str(), std::fstream::out);
            Report::writeCSVHead(*fileHandles[pubUUID]);
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
		senderReport << "\"Payload Compressibility\", \"Bytes Sent\", \"Packets sent\"" << std::endl;

		std::list<Report>::iterator senderRepIter = senderReports.begin();
		while(senderRepIter!= senderReports.end()) {
            senderReport << compressionActualRatio << ", ";
            senderReport << senderRepIter->bytesRcvd << ", ";
			senderReport << senderRepIter->pktsRcvd << std::endl;
			senderRepIter++;

			reportIter = allReports.begin();
			while(reportIter != allReports.end()) {

				if (reportIter->second.size() == 0) {
					// all data consumed - insert missing at end
					*fileHandles[reportIter->first] << "-" << std::endl;
					break;
				}

				if (reportIter->second.front().pertainingPacket > senderRepIter->pertainingPacket) {
					// no data for reporting period!
					*fileHandles[reportIter->first] << "-" << std::endl;
				} else {
					reportIter->second.front().writeCSVData(*fileHandles[reportIter->first]);
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

		std::map<std::string, std::ofstream*>::iterator fhIter = fileHandles.begin();
		while(fhIter != fileHandles.end()) {
			fhIter->second->close();
			delete(fhIter->second);
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
	}
}
