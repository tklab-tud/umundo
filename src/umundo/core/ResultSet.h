/**
 *  @file
 *  @brief      Generic interface for adding, changing, removing notification.
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


#ifndef RESULTSET_H_D2O6OBDA
#define RESULTSET_H_D2O6OBDA

#include "umundo/core/Common.h"
#include <map>

namespace umundo {

template<class T>
/**
 * Templated interface to be notified about addition, removal or changes of objects.
 */
class UMUNDO_API ResultSet {
public:
	virtual ~ResultSet() {}

	void add(T entity) {
		add(entity, "manual");
	}

	void add(T entity, const std::string& via) {
		if (_results.count(entity) == 0) {
			// we did not know about this entity!
			added(entity);
		} else {
			UM_LOG_WARN("Not adding entity into resultset - already known");
		}
		_results.insert(std::pair<T, std::string>(entity, via));
	}

	void remove(T entity) {
		remove(entity, "manual");
	}

	void remove(T entity, const std::string& via) {
		bool wasFound = false;
		typedef typename std::multimap<T, std::string>::iterator iterator;
		std::pair<iterator, iterator> iterpair = _results.equal_range(entity);
		iterator it = iterpair.first;
		for (; it != iterpair.second; ++it) {
			if (it->second == via) {
				_results.erase(it);
				wasFound = true;
				break;
			}
		}

		if (!wasFound) {
			UM_LOG_WARN("Not removing unknown entity from resultset");
			return;
		}

		if (_results.count(entity) == 0) {
			// last reports of this entity are gone
			removed(entity);
		}
	}

	void change(T entity, uint64_t what = 0) {
		change(entity, "manual", what);
	}

	void change(T entity, const std::string& via, uint64_t what = 0) {
		if (_results.count(entity) == 0) {
			UM_LOG_WARN("Not forwarding changes from unknown entity");
			return;
		}
		changed(entity, what);
	}

protected:
	std::multimap<T, std::string> _results;

	virtual void added(T) = 0;
	virtual void removed(T) = 0;
	virtual void changed(T, uint64_t what = 0) = 0;
};
}

#endif /* end of include guard: RESULTSET_H_D2O6OBDA */
