/**
 *  @file
 *  @brief      Base class for all facade implementors.
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

#ifndef IMPLEMENTATION_H_NZ2M9TXJ
#define IMPLEMENTATION_H_NZ2M9TXJ

#include "umundo/core/Common.h"

namespace umundo {
class Options;

/**
 * Abstract base class for concrete implementations in the Factory.
 *
 * Concrete implementors are registered at program initialization at the Factory and
 * instantiated for every Abstraction that needs an implementation.
 */
class UMUNDO_API Implementation {
public:
	Implementation() : _isSuspended(false) {}
	/** @name Life Cycle Management */
	//@{
	virtual void init(const Options*) = 0; ///< initialize instance after creation
	virtual void suspend() {}; ///< Optional hook to suspend implementations
	virtual void resume() {}; ///< Optional hook to resume implementations
	//@}

protected:
	bool _isSuspended;

private:
	virtual SharedPtr<Implementation> create() = 0; ///< Factory method called by the Factory class
	friend class Factory; ///< In C++ friends can see your privates!
};

/**
 * Abstract base class for options of Implementation%s.
 *
 * C++ does not allow Implementation::init() to take non-PODs as a variadic function, we need
 * some way to abstract option data. We could also pass it to Factory::create(), but I
 * like the idea of a dedicated init phase.
 */
class UMUNDO_API Options {
public:
	virtual ~Options() {}
	virtual std::string getType() {
		return "";
	}; // TODO: do we actually use this?!

	std::map<std::string, std::string> getKVPs() const {
		return options;
	}

protected:
	std::map<std::string, std::string> options;
};
}

#endif /* end of include guard: IMPLEMENTATION_H_NZ2M9TXJ */
