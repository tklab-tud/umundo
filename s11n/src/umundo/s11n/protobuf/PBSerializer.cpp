/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
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
 *  @endcond
 */

#include "PBSerializer.h"
#include "umundo/common/Factory.h"
#include "umundo/config.h"
#include <fstream>
#include <sys/stat.h>
#include <google/protobuf/descriptor.pb.h>
#ifndef WIN32
#include <dirent.h>
#else
#include <strsafe.h>
#define S_ISREG(B) ((B)&_S_IFREG)
#define S_ISDIR(B) ((B)&_S_IFDIR)
#endif


namespace umundo {
  
PBSerializer::PBSerializer() {}

PBSerializer::~PBSerializer() {}

shared_ptr<Implementation> PBSerializer::create(void*) {
  shared_ptr<Implementation> instance(new PBSerializer());
  return instance;
}

void PBSerializer::destroy() {
  
}

void PBSerializer::init(shared_ptr<Configuration> config) {
}

string PBSerializer::serialize(const string& type, void* obj) {
  MessageLite* pbObj = (MessageLite*)obj;
  //  MessageLite* pbObj = _serializers[type]->New();
  //  pbObj->CheckTypeAndMergeFrom(*((MessageLite*)obj));
  return pbObj->SerializeAsString();
}

void PBSerializer::registerType(const string& type, void* serializer) {
  _serializers[type] = (MessageLite*)serializer;
}

Mutex PBSerializer::protoMutex;
google::protobuf::DescriptorPool* PBSerializer::descPool = NULL;
google::protobuf::DynamicMessageFactory* PBSerializer::descFactory = NULL;
std::map<std::string, const google::protobuf::Descriptor*> PBSerializer::descs;
std::map<std::string, google::protobuf::compiler::Importer*> PBSerializer::descImporters;
PBErrorReporter* PBSerializer::errorReporter = NULL;
google::protobuf::compiler::DiskSourceTree* PBSerializer::sourceTree = NULL;

const google::protobuf::Message* PBSerializer::getProto(const std::string& type) {
	ScopeLock lock(protoMutex);
  // prefer descriptions from .proto files
  if (descFactory != NULL && descs.find(type) != descs.end()) {
    return descFactory->GetPrototype(descs[type]);
  }
  // otherwise take from .desc files
  if (descFactory != NULL && descPool != NULL) {
    const google::protobuf::Descriptor* desc = descPool->FindMessageTypeByName(type);
    if (desc != NULL) {
      return descFactory->GetPrototype(desc);
    }
  }
  return NULL;
}

/**
 * Add a directory or file with .desc or .proto files from the protobuf compiler.
 *
 * Be aware that we cannot deserialize objects from the actual types, but only generic
 * messages from these descriptions. That is you cannot cast to um.s11n.type in
 * receive if all we know is the description. You will have to use the protobuf
 * message interface. Register the type explicitly via registerType at the
 * TypedSubscriber if you want to cast.
 */
void PBSerializer::addProto(const std::string& dirOrFile) {
	ScopeLock lock(protoMutex);
  if (descPool == NULL) {
    descPool = new google::protobuf::DescriptorPool();
    descFactory = new google::protobuf::DynamicMessageFactory(descPool);
		sourceTree = new google::protobuf::compiler::DiskSourceTree();
		errorReporter = new PBErrorReporter();
  }

  std::string dirRoot;
  std::string relDirOrFile;
  size_t lastPathSep;
  
  if (isDir(dirOrFile)) {
    dirRoot = dirOrFile;
    relDirOrFile = "";
  } else {
    if ((lastPathSep = dirOrFile.find_last_of(PATH_SEPERATOR)) != std::string::npos) {
      relDirOrFile = dirOrFile.substr(lastPathSep, dirOrFile.length() - lastPathSep);
      dirRoot = dirOrFile.substr(0, lastPathSep);
    }
  }

	if (descImporters.find(dirRoot) == descImporters.end()) {
  	sourceTree->MapPath("", dirRoot);
  	descImporters[dirRoot] = new google::protobuf::compiler::Importer(sourceTree, errorReporter);
	}
  
  addProtoRecurse(dirRoot, relDirOrFile, descImporters[dirRoot]);
}


void PBSerializer::addProtoRecurse(const std::string& dirRoot, const std::string& dirOrFile, google::protobuf::compiler::Importer* importer) {
  
  if (dirOrFile.length() > 0 && dirOrFile.find_last_of(".") == dirOrFile.size() - 1)
    return;
    
  if (isDir(dirRoot + PATH_SEPERATOR + dirOrFile)) {
    // a directory was given
    
#ifndef WIN32
    DIR *dp;
    dp = opendir((dirRoot + PATH_SEPERATOR + dirOrFile).c_str());
    if (dp == NULL) {
      LOG_ERR("Error opening directory '%s': %s", dirOrFile.c_str(), strerror(errno));
      return;
    }
    struct dirent* entry;
    while((entry = readdir(dp))) {
      addProtoRecurse(dirRoot, dirOrFile + PATH_SEPERATOR + entry->d_name, importer);
    }
    closedir(dp);
#else
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR szDir[MAX_PATH];
    StringCchCopy(szDir, MAX_PATH, (dirRoot + PATH_SEPERATOR + dirOrFile).c_str());
    StringCchCat(szDir, MAX_PATH, TEXT("\\*"));
    hFind = FindFirstFile(szDir, &ffd);
    do {
      addProtoRecurse(dirRoot, dirOrFile + PATH_SEPERATOR + ffd.cFileName, importer);
    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);
#endif
    return;
  }
  
  if (dirOrFile.length() > 6 && dirOrFile.substr(dirOrFile.length() - 6, 6).compare(".proto") == 0) {
    const google::protobuf::FileDescriptor* fileDesc = importer->Import(dirOrFile.substr(1, dirOrFile.length() - 1));
    if (fileDesc != NULL) {
      for (int j = 0; j < fileDesc->message_type_count(); j++) {
        LOG_INFO("Added generic description for serializable type %s from .proto file", fileDesc->message_type(j)->name().c_str());
        descs[fileDesc->message_type(j)->name()] = fileDesc->message_type(j);
      }
    } else {
      LOG_ERR("Could not parse %s as .proto file", dirOrFile.c_str());
    }
  } else {
    std::ifstream file((dirRoot + PATH_SEPERATOR + dirOrFile).c_str(), std::ios::in);
    std::string content(static_cast<std::stringstream const&>(std::stringstream() << file.rdbuf()).str());
    
    google::protobuf::FileDescriptorSet fdSet;
    fdSet.ParseFromString(content);
    for (int i = 0; i < fdSet.file_size(); i++) {
      google::protobuf::FileDescriptorProto fdProto = fdSet.file(i);
      for (int j = 0; j < fdProto.dependency_size(); j++) {
        std::string depProto = fdProto.dependency(j);
        addProto(depProto);
      }
      for (int j = 0; j < fdProto.message_type_size(); j++) {
        LOG_INFO("Added generic description for serializable type %s form .desc file", fdProto.message_type(j).name().c_str());
      }
      descPool->BuildFile(fdProto);
    }
  }
}

bool PBSerializer::isDir(const std::string& dirOrFile) {
  int status;
  struct stat dirStat;
  status = stat (dirOrFile.c_str(), &dirStat);
  
  if (status != 0) {
    LOG_ERR("Cannot open %s: %s", dirOrFile.c_str(), strerror(errno));
    return false;
  }
  return((bool)(S_ISDIR (dirStat.st_mode)));
}
  
void PBErrorReporter::AddError(const string & filename, int line, int column, const string & message) {
	LOG_ERR("filename %s:%d:%d: %s", filename.c_str(), line, column, message.c_str());
}
  
}