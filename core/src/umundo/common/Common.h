/**
 *  @file
 *  @brief      Set pragmas, handle DLLEXPORT and import symbols into umundo namespace.
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

#ifndef COMMON_H_ANPQOWX0
#define COMMON_H_ANPQOWX0

// disable: "<type> needs to have dll-interface to be used by clients'
// Happens on STL member variables which are not public therefore is ok?
//#pragma warning (disable : 4251)

// see
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q172396
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q168958
// http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
// http://stackoverflow.com/questions/1881494/how-to-expose-stl-list-over-dll-boundary
// http://stackoverflow.com/questions/5661738/common-practice-in-dealing-with-warning-c4251-class-needs-to-have-dll-inter/

#if defined(_MSC_VER)
// disable signed / unsigned comparison warnings
#pragma warning (disable : 4018)
// possible loss of data
#pragma warning (disable : 4244)
#pragma warning (disable : 4267)
// 'this' : used in base member initializer list (TypedSubscriber)
#pragma warning (disable : 4355)

#endif

#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

// #if __cplusplus > 199711L
// 	typedef boost::shared_ptr std::shared_ptr
// 	typedef boost::weak_ptr std::weak_ptr
// #else
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
// #endif
#include <stdlib.h>
#include <stdint.h>

#if defined(_WIN32) && !defined(UMUNDO_STATIC)
#	ifdef COMPILING_DLL
#		define DLLEXPORT __declspec(dllexport)
#	else
#		define DLLEXPORT __declspec(dllimport)
#	endif
#else
#	define DLLEXPORT
#endif

// #if defined UNIX || defined IOS || defined IOSSIM
// #include <string.h> // memcpy
// #include <stdio.h> // snprintf
// #endif

#include "portability.h"
#include "umundo/common/Debug.h"

namespace umundo {

extern std::string procUUID;
extern std::string hostUUID;

inline bool isNumeric( const char* pszInput, int nNumberBase) {
	std::string base = ".-0123456789ABCDEF";
	std::string input = pszInput;
	return (input.find_first_not_of(base.substr(0, nNumberBase + 2)) == std::string::npos);
}

// see http://stackoverflow.com/questions/228005/alternative-to-itoa-for-converting-integer-to-string-c
template <typename T> std::string toStr(T tmp) {
	std::ostringstream out;
	out << tmp;
	return out.str();
}

template <typename T> T strTo(std::string tmp) {
	T output;
	std::istringstream in(tmp);
	in >> output;
	return output;
}

template <typename S, typename T>
size_t unique_keys(std::multimap<S, T> mm) {
	size_t uniqueCount = 0;
	typename std::multimap<S, T>::iterator end;
	typename std::multimap<S, T>::iterator it;
	for(it = mm.begin(), end = mm.end();
	        it != end;
	        it = mm.upper_bound(it->first)) {
		uniqueCount++;
	}
	return uniqueCount;
}

}

#endif /* end of include guard: COMMON_H_ANPQOWX0 */

/**
 * \mainpage umundo-core
 *
 * This is the documentation of umundo-core, a leight-weight implementation of a pub/sub system. Its only responsibility
 * is to deliver byte-arrays from publishers to subscribers on channels. Where a channel is nothing more than an agreed
 * upon ASCII string.
 */
