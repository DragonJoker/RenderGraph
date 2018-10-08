#include "Utils/Miscellaneous/PreciseTimer.hpp"

#include "Utils/Miscellaneous/Utils.hpp"

namespace utils
{
	PreciseTimer::PreciseTimer()
	{
		m_savedTime = doGetElapsed();
	}

	PreciseTimer::~PreciseTimer()
	{
	}

	Nanoseconds PreciseTimer::getElapsed()
	{
		auto current = doGetElapsed();
		auto diff( current - m_savedTime );
		m_savedTime = current;
		return std::chrono::duration_cast< Nanoseconds >( diff );
	}

	PreciseTimer::clock::time_point PreciseTimer::doGetElapsed()const
	{
		return clock::now();
	}
}
