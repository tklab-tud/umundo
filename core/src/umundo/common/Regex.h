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

#ifndef REGEX_H_HUE92M
#define REGEX_H_HUE92M

#include <umundo/common/Common.h>
#include <pcre.h>
#include <string>
#include <vector>

#define OVECCOUNT 30    /* should be a multiple of 3 */

namespace umundo {

using std::string;

/**
 * Match a regular expression against a string.
 *
 * Matches are expressed as pairs of offset and length.
 */
class DLLEXPORT Regex {
public:
	Regex(const string&);
	
  bool hasError();
	bool matches(const string&);
	void setPattern(const string&);
  
  const string getPattern()                         { return _pattern; }
  bool hasSubMatches()                              { return _subMatchIndex.size() > 0; }
  std::pair<int, int> getMatch()                    { return _matchIndex; }
  std::vector<std::pair<int, int> > getSubMatches() { return _subMatchIndex; }
  
protected:
  int _nrMatches;
  pcre* _re;
	string _pattern;
  int _ovector[OVECCOUNT];
  std::pair<int, int> _matchIndex;
  std::vector<std::pair<int, int> > _subMatchIndex;
  
  string _error;
  int _errorOffset;
};
	
}

#endif /* end of include guard: REGEX_H_HUE92M */
