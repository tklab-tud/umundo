#include "umundo/cache/CacheProfiler.h"
#include <fstream>
#include <iomanip>

namespace umundo {

CacheProfiler::CacheProfiler(SCache* cache, const string& filename) {
  _cache = cache;
  _filename = filename;
  _filesWritten = 0;
}
  
CacheProfiler::~CacheProfiler() {

}

void CacheProfiler::startedPressureBalancing(float currentPressure) {
  
}
  
void CacheProfiler::finishedPressureBalancing(float currentPressure) {
  _allPressureBounds.push_back(_currPressureBounds);
  _currPressureBounds.clear();
}
  
void CacheProfiler::newBounds(float lowerBound, float upperBound, float currPressure, uint64_t proposedSize) {
  PressureBound currBound;
  currBound.lowerBound = lowerBound;
  currBound.upperBound = upperBound;
  currBound.propsedSize = proposedSize;
  currBound.pressure = currPressure;
  currBound.maxSize = _cache->_maxSize;

  _currPressureBounds.push_back(currBound);
}

void CacheProfiler::writeToPlot() {
  vector<vector<PressureBound> >::iterator iterIter = _allPressureBounds.begin();
  while(iterIter != _allPressureBounds.end()) {
    
    std::ofstream outFile;
    std::stringstream outFileName;
    outFileName << _filename << "." << std::setw(2) << std::setfill('0') << _filesWritten << ".bounds.dat";
    outFile.open(outFileName.str().c_str());
    outFile << "#iteration, pressure, lower bound, upper bound, proposed size, max size" << std::endl;
    
    vector<PressureBound>::iterator boundIter = iterIter->begin();
    int iter = 0;
    while(boundIter != iterIter->end()) {
      outFile << iter++
      << " " << boundIter->pressure 
      << " " << boundIter->lowerBound 
      << " " << boundIter->upperBound 
      << " " << boundIter->propsedSize 
      << " " << boundIter->maxSize 
      << std::endl;
      boundIter++;
    }
    
//    std::ofstream plotFile;
//    std::stringstream plotFileName;
//    plotFileName << _filename << "." << std::setw(2) << std::setfill('0') << _filesWritten << ".plot";
//    plotFile.open(plotFileName.str().c_str());
//    
//    plotFile << "plot " << "\"" << outFileName.str() << "\" using 1 title \"lower bound\" with linespoints" << 
//      ", " << "\"" << outFileName.str() << "\" using 2 title \"upper bound\" with linespoints" <<
//      ", " << "\"" << outFileName.str() << "\" using 3 title \"pressure\" with linespoints" << std::endl;
//    
//    plotFile.close();
    outFile.close();
    iterIter++;
    _filesWritten++;
  }  
}

  
void CacheProfiler::layoutToDot(SCache* cache, const string& filename, const CacheItemLabeler& labeler) {
  std::ofstream outFile;
  outFile.open(filename.c_str());
  outFile << "digraph \"Digits\" {" << std::endl;
  outFile << "compound=true;" << std::endl;
  outFile << "pack=true;" << std::endl;
  outFile << "rankdir = LR;" << std::endl;

  string graphLabel = labeler.caption(cache);
  if (graphLabel.length() > 0) {
    outFile << "label=\"" << graphLabel << "\"" << std::endl;
    outFile << "labelloc=top" << std::endl;
    outFile << "labeljust=left" << std::endl;
  }
  
  set<weak_ptr<SCachePointer> >::iterator ptrIter = cache->_cachePointers.begin();
	while(ptrIter != cache->_cachePointers.end()) {
		shared_ptr<SCachePointer> ptr = ptrIter->lock();
		if (ptr.get() != NULL) {
      outFile << "\tnode" << (intptr_t)ptr.get() << " [ label=\"Ptr\", labeljust=l, color=blue, penwidth=1]" << std::endl;
      outFile << "\tnode" << (intptr_t)ptr.get() << " -> node" << (intptr_t)(ptr->_item) << " [label=\"\"]" << std::endl;
		}
    ptrIter++;
  }
  
  set<SCacheItem*>::iterator itemIter = cache->_cacheItems.begin();
  while(itemIter != cache->_cacheItems.end()) {
    outFile << "\tnode" << (intptr_t)(*itemIter) 
            << " [ label=\"" << labeler.label((*itemIter)) 
            << "\", labeljust=l, color=" << labeler.shapeColor((*itemIter)) 
            << ", penwidth=" << labeler.penWidth((*itemIter)) << "]" << std::endl;
    set<SCacheItem*> next = (*itemIter)->getNext();
    set<SCacheItem*>::iterator nextIter = next.begin();
    while(nextIter != next.end()) {
      if ((*nextIter) != NULL)
        outFile << "\tnode" << (intptr_t)(*itemIter) << " -> node" << (intptr_t)*nextIter << " [label=\"" << labeler.label((*itemIter), *nextIter) << "\"]" << std::endl;
      nextIter++;
    }
    outFile << std::endl;
    itemIter++;
  }
  
  outFile << "}" << std::endl;
  outFile.close();
}
  
}