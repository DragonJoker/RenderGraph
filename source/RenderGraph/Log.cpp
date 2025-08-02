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

		static void doLog( std::string_view message
			, bool newLine
			, FILE * stream )noexcept
		{
			fprintf( stream, "%s", message.data() );

			if ( newLine )
			{
				fprintf( stream, "\n" );
			}
		}
	}

	Logger::Logger()
		: m_trace{ []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); } }
		, m_debug{ []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); } }
		, m_info{ []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); } }
		, m_warning{ []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); } }
		, m_error{ []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stderr ); } }
	{
	}

	void Logger::logTrace( std::string_view message, bool newLine )noexcept
	{
		doGetInstance().m_trace( message, newLine );
	}

	void Logger::logDebug( std::string_view message, bool newLine )noexcept
	{
		doGetInstance().m_debug( message, newLine );
	}

	void Logger::logInfo( std::string_view message, bool newLine )noexcept
	{
		doGetInstance().m_info( message, newLine );
	}

	void Logger::logWarning( std::string_view message, bool newLine )noexcept
	{
		doGetInstance().m_warning( message, newLine );
	}

	void Logger::logError( std::string_view message, bool newLine )noexcept
	{
		doGetInstance().m_error( message, newLine );
	}

	void Logger::setTraceCallback( LogCallback callback )
	{
		if ( callback )
			doGetInstance().m_trace = std::move( callback );
		else
			doGetInstance().m_trace = []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); };
	}

	void Logger::setDebugCallback( LogCallback callback )
	{
		if ( callback )
			doGetInstance().m_debug = std::move( callback );
		else
			doGetInstance().m_debug = []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); };
	}

	void Logger::setInfoCallback( LogCallback callback )
	{
		if ( callback )
			doGetInstance().m_info = std::move( callback );
		else
			doGetInstance().m_info = []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); };
	}

	void Logger::setWarningCallback( LogCallback callback )
	{
		if ( callback )
			doGetInstance().m_warning = std::move( callback );
		else
			doGetInstance().m_warning = []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stdout ); };
	}

	void Logger::setErrorCallback( LogCallback callback )
	{
		if ( callback )
			doGetInstance().m_error = std::move( callback );
		else
			doGetInstance().m_error = []( std::string_view msg, bool newLine )noexcept { log::doLog( msg, newLine, stderr ); };
	}

	Logger & Logger::doGetInstance()noexcept
	{
		static Logger instance;
		static std::mutex mutex;

		log::lock_type lock{ mutex };
		return instance;
	}
}
