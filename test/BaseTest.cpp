#include "BaseTest.hpp"

#include <RenderGraph/Log.hpp>

#if defined( _WIN32 )
#	include <Windows.h>
#elif defined( __APPLE__ )
#	include <string>
#	include <vector>
#	include <mach-o/dyld.h>
#elif defined( __linux__ )
#	include <unistd.h>
#	include <dirent.h>
#	include <pwd.h>
#endif

#include <array>
#include <functional>
#include <iomanip>
#include <map>
#include <sstream>
#include <set>

namespace test
{
	using StringArray = std::vector< std::string >;

	//*********************************************************************************************

	namespace
	{
		template< typename LogStreambufTraits >
		class LogStreambuf
			: public std::streambuf
		{
		public:
			using string_type = std::string;
			using ostream_type = std::ostream;
			using streambuf_type = std::streambuf;
			using int_type = std::streambuf::int_type;
			using traits_type = std::streambuf::traits_type;

			LogStreambuf( LogStreambuf const & ) = delete;
			LogStreambuf & operator=( LogStreambuf const & ) = delete;
			LogStreambuf( LogStreambuf && ) = delete;
			LogStreambuf & operator=( LogStreambuf && ) = delete;

			explicit inline LogStreambuf( std::string const & name
				, std::ostream & stream )
				: m_stream{ stream }
				, m_fstream{ getExecutableDirectory() + name + ".log" }
			{
				m_old = m_stream.rdbuf( this );
			}

			inline ~LogStreambuf()override
			{
				m_stream.rdbuf( m_old );
			}

			inline int_type overflow( int_type c = traits_type::eof() )override
			{
				if ( traits_type::eq_int_type( c, traits_type::eof() ) )
				{
					do_sync();
				}
				else if ( c == '\n' )
				{
					do_sync();
				}
				else if ( c == '\r' )
				{
					m_buffer += '\r';
					do_sync_no_nl();
				}
				else
				{
					m_buffer += traits_type::to_char_type( c );
				}

				return c;
			}

			inline int do_sync()
			{
				LogStreambufTraits::log( m_fstream, m_buffer );
				m_buffer.clear();
				return 0;
			}

			inline int do_sync_no_nl()
			{
				LogStreambufTraits::logNoNL( m_fstream, m_buffer );
				m_buffer.clear();
				return 0;
			}

		private:
			string_type m_buffer;
			ostream_type & m_stream;
			streambuf_type * m_old;
			std::ofstream m_fstream;
		};

		struct DebugLogStreambufTraits
		{
			static inline void log( std::ostream & stream
				, std::string const & text )
			{
				stream << "DEBUG: " << text << std::endl;
				printf( "%s\n", text.c_str() );
			}

			static inline void logNoNL( std::ostream & stream
				, std::string const & text )
			{
				stream << "DEBUG: " << text;
				printf( "%s", text.c_str() );
			}
		};

		struct InfoLogStreambufTraits
		{
			static inline void log( std::ostream & stream
				, std::string const & text )
			{
				stream << text << std::endl;
				printf( "%s\n", text.c_str() );
			}

			static inline void logNoNL( std::ostream & stream
				, std::string const & text )
			{
				stream << text;
				printf( "%s", text.c_str() );
			}
		};

		struct ErrorLogStreambufTraits
		{
			static inline void log( std::ostream & stream
				, std::string const & text )
			{
				stream << "ERROR: " << text << std::endl;
				printf( "%s\n", text.c_str() );
			}

			static inline void logNoNL( std::ostream & stream
				, std::string const & text )
			{
				stream << "ERROR: " << text;
				printf( "%s", text.c_str() );
			}
		};

#if defined( _WIN32 )
		static char constexpr PathSeparator = '\\';
#else
		static char constexpr PathSeparator = '/';
#endif

		std::string getPath( std::string const & path )
		{
			return path.substr( 0, path.find_last_of( PathSeparator ) );
		}

		std::string findMissing( StringArray const & lhsLines
			, StringArray const & rhsLines
			, std::string const & op )
		{
			std::string result;

			for ( auto & lhsLine : lhsLines )
			{
				if ( rhsLines.end() == std::find( rhsLines.begin()
					, rhsLines.end()
					, lhsLine ) )
				{
					result += op + lhsLine + "\n";
				}
			}

			return result;
		}
	}

	//*********************************************************************************************

#if defined( _WIN32 )

	std::string getExecutableDirectory()
	{
		std::string result;
		std::array< char, FILENAME_MAX > path{};
		DWORD res = ::GetModuleFileNameA( nullptr
			, path.data()
			, sizeof( path ) );

		if ( res != 0 )
		{
			result = path.data();
		}

		result = getPath( result ) + PathSeparator;
		return result;
	}

#elif defined( __APPLE__ )

	std::string getExecutableDirectory()
	{
		std::string result;
		std::array< char, FILENAME_MAX > path{};
		uint32_t size = FILENAME_MAX;

		if ( _NSGetExecutablePath( path.data(), &size ) == 0 )
		{
			char realPath[FILENAME_MAX]{};
			realpath( path.data(), realPath );
			result = std::string{ realPath };
		}

		result = getPath( result ) + PathSeparator;
		return result;
	}
	
#else

	std::string getExecutableDirectory()
	{
		std::string result;
		std::array< char, FILENAME_MAX > path{};
		char buffer[32];
		sprintf( buffer, "/proc/%d/exe", getpid() );
		int bytes = std::min< std::size_t >( readlink( buffer
			, path.data()
			, path.size() )
			, path.size() - 1 );

		if ( bytes > 0 )
		{
			path[bytes] = '\0';
			result = path.data();
		}

		result = getPath( result ) + PathSeparator;
		return result;
	}

#endif

	//*********************************************************************************************

	StringArray splitInLines( std::string const & value )
	{
		StringArray lines;
		std::stringstream iss( value );

		while ( iss.good() )
		{
			std::string line;
			std::getline( iss, line );
			lines.push_back( line );
		}

		return lines;
	}

	std::string TestCounts::diffLines( std::string const & check
		, std::string const & ref )
	{
		auto checkLines = splitInLines( check );
		auto refLines = splitInLines( ref );
		std::string result;
		result += findMissing( checkLines, refLines, "+" );
		result += findMissing( refLines, checkLines, "-" );
		return result;
	}

	//*********************************************************************************************

	TestSuite::TestSuite( std::string const & name )
		: tclog{ std::make_unique< test::LogStreambuf< test::DebugLogStreambufTraits > >( name, std::clog ) }
		, tcout{ std::make_unique< test::LogStreambuf< test::InfoLogStreambufTraits > >( name, std::cout ) }
		, tcerr{ std::make_unique< test::LogStreambuf< test::ErrorLogStreambufTraits > >( name, std::cerr ) }
	{
		crg::Logger::setTraceCallback( []( std::string_view, bool )noexcept
			{
				// Don't log trace
			} );
		crg::Logger::setDebugCallback( []( std::string_view msg, bool newLine )noexcept
			{
				std::clog << msg.data();
				if ( newLine )
					std::clog << "\n";
			} );
		crg::Logger::setInfoCallback( []( std::string_view msg, bool newLine )noexcept
			{
				std::cout << msg.data();
				if ( newLine )
					std::cout << "\n";
			} );
		crg::Logger::setWarningCallback( []( std::string_view msg, bool newLine )noexcept
			{
				std::cout << msg.data();
				if ( newLine )
					std::cout << "\n";
			} );
		crg::Logger::setErrorCallback( []( std::string_view msg, bool newLine )noexcept
			{
				std::cerr << msg.data();
				if ( newLine )
					std::cerr << "\n";
			} );
	}

	//*********************************************************************************************

	int testsMain( int argc, char ** argv, std::string_view testSuiteName )
	{
		std::locale::global( std::locale{ "C" } );
		testing::InitGoogleTest( &argc, argv );
		auto suite = new test::TestSuite{ std::string{ testSuiteName } };
		testing::AddGlobalTestEnvironment( suite );
		return RUN_ALL_TESTS();
	}

	std::string sortLines( std::string const & value )
	{
		std::stringstream stream{ value };
		std::multiset< std::string > sorted;
		std::string line;

		while ( std::getline( stream, line ) )
		{
			sorted.insert( line );
		}

		std::stringstream result;

		for ( auto & v : sorted )
		{
			result << v << std::endl;
		}

		return result.str();
	}

	//*********************************************************************************************
}
