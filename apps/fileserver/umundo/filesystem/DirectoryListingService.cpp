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

//#include "umundo/config.h"

#include "umundo/filesystem/DirectoryListingService.h"
#include "umundo/util/crypto/MD5.h"
#include "umundo/common/Regex.h"

#ifndef WIN32
#include <dirent.h>
#else
#include <strsafe.h>
#endif

#define ATIME_TO_MS(stat) (uint64_t)stat.st_atime * 1000
#define CTIME_TO_MS(stat) (uint64_t)stat.st_ctime * 1000
#define MTIME_TO_MS(stat) (uint64_t)stat.st_mtime * 1000
#ifdef HAS_STAT_BIRTHTIME
# define BTIME_TO_MS(stat) (uint64_t)stat.st_btime * 1000
#else
# define BTIME_TO_MS(stat) 0
#endif

#include <stdio.h>
#include <string.h>

namespace umundo {

DirectoryListingService::DirectoryListingService(const string& directory) {
	_dir = directory;
	_notifyPub = new TypedPublisher(_channelName + ":notify");
	_notifyPub->registerType("DirectoryEntry", new DirectoryEntry());
	_notifyPub->registerType("DirectoryEntryContent", new DirectoryEntryContent());
	start();
}

DirectoryListingService::~DirectoryListingService() {
	stop();
	join();
}

std::set<umundo::Publisher*> DirectoryListingService::getPublishers() {
	set<Publisher*> pubs = Service::getPublishers();
	pubs.insert(_notifyPub);
	return pubs;
}

void DirectoryListingService::run() {
	while(isStarted()) {
		updateEntries();
		Thread::sleepMs(500);
	}
}

DirectoryListingReply* DirectoryListingService::list(DirectoryListingRequest* req) {
	updateEntries();
	DirectoryListingReply* reply = new DirectoryListingReply();
	map<string, struct stat>::iterator statIter = _knownEntries.begin();

	Regex regex(req->pattern());
	if (regex.hasError())
		return reply;

	while(statIter != _knownEntries.end()) {
		DirectoryEntry* dOrig = statToEntry(statIter->first, statIter->second);
		if (regex.matches(statIter->first)) {
			DirectoryEntry* dEntry = reply->add_entries();
			dEntry->CopyFrom(*dOrig);
			delete dOrig;
		}
		statIter++;
	}
	return reply;
}

DirectoryEntryContent* DirectoryListingService::get(DirectoryEntry* req) {
	DirectoryEntryContent* rep = new DirectoryEntryContent();

  if (_knownEntries.find(req->name()) == _knownEntries.end()) {
		LOG_ERR("Request for unkown file '%s'", req->name().c_str());
		return rep;
  }
  
	struct stat fileStat;
	if (stat((_dir + '/' +  req->name()).c_str(), &fileStat) != 0) {
		LOG_ERR("Error with stat on '%s': %s", req->name().c_str(), strerror(errno));
		return rep;
	}
	char* buffer = readFileIntoBuffer(req->name().c_str(), fileStat.st_size);
	rep->set_content(buffer, fileStat.st_size);
	rep->set_md5(md5(buffer, fileStat.st_size));
	free(buffer);
	return rep;
}

void DirectoryListingService::notifyNewFile(const string& fileName, struct stat fileStat) {
	LOG_DEBUG("New file %s found", fileName.c_str());
	DirectoryEntry* dirEntry = statToEntry(fileName, fileStat);
	Message* msg = _notifyPub->prepareMsg("DirectoryEntry", dirEntry);
	msg->putMeta("operation", "added");

	_notifyPub->send(msg);

	delete msg;
	delete dirEntry;
}

void DirectoryListingService::notifyModifiedFile(const string& fileName, struct stat fileStat) {
	LOG_DEBUG("New file %s found", fileName.c_str());
	DirectoryEntry* dirEntry = statToEntry(fileName, fileStat);
	Message* msg = _notifyPub->prepareMsg("DirectoryEntry", dirEntry);
	msg->putMeta("operation", "modified");

	_notifyPub->send(msg);

	delete msg;
	delete dirEntry;
}

void DirectoryListingService::notifyRemovedFile(const string& fileName, struct stat fileStat) {
	LOG_DEBUG("Removed file %s", fileName.c_str());

	DirectoryEntry* dirEntry = statToEntry(fileName, fileStat);
	Message* msg = _notifyPub->prepareMsg("DirectoryEntry", dirEntry);
	msg->putMeta("operation", "removed");

	_notifyPub->send(msg);

	delete msg;
	delete dirEntry;
}

void DirectoryListingService::updateEntries() {
	ScopeLock lock(&_mutex);

	// stat directory for modification date
	struct stat dirStat;
	if (stat(_dir.c_str(), &dirStat) != 0) {
		LOG_ERR("Error with stat on directory '%s': %s", _dir.c_str(), strerror(errno));
		return;
	}

	if ((unsigned)dirStat.st_mtime >= (unsigned)_lastChecked) {
		// there are changes in the directory
		set<string> currEntries;

#ifndef WIN32
		DIR *dp;
		dp = opendir(_dir.c_str());
		if (dp == NULL) {
			LOG_ERR("Error opening directory '%s': %s", _dir.c_str(), strerror(errno));
			return;
		}
		// iterate all entries and see what changed
		struct dirent* entry;
		while((entry = readdir(dp))) {
			string dname = entry->d_name;
#else
		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		TCHAR szDir[MAX_PATH];
		StringCchCopy(szDir, MAX_PATH, _dir.c_str());
		StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

		hFind = FindFirstFile(szDir, &ffd);
		do {
			string dname = ffd.cFileName;
#endif

			// see if the file was changed
			char* filename;
			asprintf(&filename, "%s/%s", _dir.c_str(), dname.c_str());
      
			struct stat fileStat;
			if (stat(filename, &fileStat) != 0) {
				LOG_ERR("Error with stat on directory entry '%s': %s", filename, strerror(errno));
				free(filename);
				continue;
			}

			if (fileStat.st_mode & S_IFDIR) {
				// ignore directories
				free(filename);
				continue;
			}

			// are we interested in such a file?
			if (!filter(dname)) {
				free(filename);
				continue;
			}
			currEntries.insert(dname);

			if (_knownEntries.find(dname) != _knownEntries.end()) {
				// we have seen this entry before
				struct stat oldStat = _knownEntries[dname];
				if (oldStat.st_mtime < fileStat.st_mtime) {
					notifyModifiedFile(dname, fileStat);
				}
			} else {
				// we have not yet seen this entry
				notifyNewFile(dname, fileStat);
			}

			free(filename);
			_knownEntries[dname] = fileStat; // gets copied on insertion
#ifndef WIN32
		}
		closedir(dp);
#else
		}
		while (FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
#endif
		// are there any known entries we have not seen this time around?
		map<string, struct stat>::iterator fileIter = _knownEntries.begin();
		while(fileIter != _knownEntries.end()) {
			if (currEntries.find(fileIter->first) == currEntries.end()) {
				// we used to know this file
				notifyRemovedFile(fileIter->first, fileIter->second);
				_knownEntries.erase(fileIter->first);
			}
			fileIter++;
		}
		// remember when we last checked the directory for modifications
#ifndef WIN32
		time(&_lastChecked);
#else
		// TODO: this will fail with subsecond updates to the directory
		_lastChecked = dirStat.st_mtime + 1;
#endif
	}
}

DirectoryEntry* DirectoryListingService::statToEntry(const string& fileName, struct stat fileStat) {
	DirectoryEntry* dEntry = new DirectoryEntry();
	dEntry->set_name(fileName);
	dEntry->set_hostid(Host::getHostId());
	dEntry->set_path(_dir);
	dEntry->set_size(fileStat.st_size);

	dEntry->set_atime_ms(ATIME_TO_MS(fileStat));
	dEntry->set_btime_ms(BTIME_TO_MS(fileStat));
	dEntry->set_ctime_ms(CTIME_TO_MS(fileStat));
	dEntry->set_mtime_ms(MTIME_TO_MS(fileStat));

	if (false) {

	} else if (fileStat.st_mode & S_IFREG) {
		dEntry->set_type(DirectoryEntry_Type_FILE);

	} else if (fileStat.st_mode & S_IFDIR) {
		dEntry->set_type(DirectoryEntry_Type_DIR);

	} else if (fileStat.st_mode & S_IFCHR) {
		dEntry->set_type(DirectoryEntry_Type_CHAR_DEV);
#ifdef UNIX
	} else if (fileStat.st_mode & S_IFLNK) {
		dEntry->set_type(DirectoryEntry_Type_SYMLINK);

	} else if (fileStat.st_mode & S_IFSOCK) {
		dEntry->set_type(DirectoryEntry_Type_SOCKET);

	} else if (fileStat.st_mode & S_IFIFO) {
		dEntry->set_type(DirectoryEntry_Type_NAMED_PIPE);

	} else if (fileStat.st_mode & S_IFBLK) {
		dEntry->set_type(DirectoryEntry_Type_BLOCK_DEV);
#endif
	} else {
		LOG_WARN("Could not determine fs type of %s", fileName.c_str());
		dEntry->set_type(DirectoryEntry_Type_UNKNOWN);
	}

	size_t found = fileName.find_last_of(".");
	if (found) {
		dEntry->set_extension(fileName.substr(found + 1));
	}

	int start = 0;
	int end = 0;
	int segment = 0;
	while ((end = _dir.find("/\\", end)) > 0) {
		dEntry->set_segments(segment++, _dir.substr(start, end));
		start = end;
	}

	return dEntry;
}

char* DirectoryListingService::readFileIntoBuffer(const string& fileName, int size) {
	char* absFilename;
	asprintf(&absFilename, "%s/%s", _dir.c_str(), fileName.c_str());
	char* buffer = (char*)malloc(size);

	FILE* fd;
	if ((fd = fopen(absFilename, "rb")) == NULL) {
		LOG_ERR("Could not open %s: %s", absFilename, strerror(errno));
		free(buffer);
		free(absFilename);
		return NULL;
	}

	int n;
	if (size > 0) {
		if ((n = fread(buffer, 1, size, fd)) != size) {
			LOG_ERR("Expected to read %d bytes form %s, but got %d", size, absFilename, n);
			free(buffer);
			free(absFilename);
			return NULL;
		}
	}

	fclose(fd);
	free(absFilename);
	return buffer;
}

}