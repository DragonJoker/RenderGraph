#include "RenderGraph/Log.hpp"

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <iostream>
#include <sstream>
#include <mutex>
#pragma warning( pop )

namespace crg
{
	namespace log
	{
		using lock_type = std::unique_lock< std::mutex >;

		static void doLog( std::string const & message
			, bool newLine
			, std::ostream & stream )
		{
			stream << message;

			if ( newLine )
			{
				stream << std::endl;
			}
		}
	}

	Logger::Logger()
		: m_trace{ []( std::string const & msg, bool newLine ){ log::doLog( msg, newLine, std::clog ); } }
		, m_debug{ []( std::string const & msg, bool newLine ){ log::doLog( msg, newLine, std::clog ); } }
		, m_info{ []( std::string const & msg, bool newLine ){ log::doLog( msg, newLine, std::cout ); } }
		, m_warning{ []( std::string const & msg, bool newLine ){ log::doLog( msg, newLine, std::cout ); } }
		, m_error{ []( std::string const & msg, bool newLine ){ log::doLog( msg, newLine, std::cerr ); } }
	{
	}

	void Logger::logTrace( std::string const & message, bool newLine )
	{
		doGetInstance().m_trace( message, newLine );
	}

	void Logger::logDebug( std::string const & message, bool newLine )
	{
		doGetInstance().m_debug( message, newLine );
	}

	void Logger::logInfo( std::string const & message, bool newLine )
	{
		doGetInstance().m_info( message, newLine );
	}

	void Logger::logWarning( std::string const & message, bool newLine )
	{
		doGetInstance().m_warning( message, newLine );
	}

	void Logger::logError( std::string const & message, bool newLine )
	{
		doGetInstance().m_error( message, newLine );
	}

	void Logger::setTraceCallback( LogCallback callback )
	{
		doGetInstance().m_trace = std::move( callback );
	}

	void Logger::setDebugCallback( LogCallback callback )
	{
		doGetInstance().m_debug = std::move( callback );
	}

	void Logger::setInfoCallback( LogCallback callback )
	{
		doGetInstance().m_info = std::move( callback );
	}

	void Logger::setWarningCallback( LogCallback callback )
	{
		doGetInstance().m_warning = std::move( callback );
	}

	void Logger::setErrorCallback( LogCallback callback )
	{
		doGetInstance().m_error = std::move( callback );
	}

	Logger & Logger::doGetInstance()
	{
		static Logger instance;
		static std::mutex mutex;

		log::lock_type lock{ mutex };
		return instance;
	}
}
