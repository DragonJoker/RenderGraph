#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformAndroid )

#	include "Utils/Config/MultiThreadConfig.hpp"
#	include "Utils/Miscellaneous/Debug.hpp"
#	include "Utils/Miscellaneous/StringUtils.hpp"

#	include <cstdlib>

namespace utils
{
	namespace Debug
	{
		template< typename CharT >
		inline void doShowBacktrace (std::basic_ostream< CharT > & p_stream, int p_toCapture, int p_toSkip)
		{
		}

		void initialise()
		{
		}

		void cleanup()
		{
		}

		void loadModule( DynamicLibrary const & library )
		{
		}

		void unloadModule( DynamicLibrary const & library )
		{
		}

		std::wostream & operator<<( std::wostream & p_stream, Backtrace const & p_backtrace )
		{
			doShowBacktrace( p_stream, p_backtrace.m_toCapture, p_backtrace.m_toSkip );
			return p_stream;
		}

		std::ostream & operator<<( std::ostream & p_stream, Backtrace const & p_backtrace )
		{
			doShowBacktrace( p_stream, p_backtrace.m_toCapture, p_backtrace.m_toSkip );
			return p_stream;
		}

	}
}

#endif
