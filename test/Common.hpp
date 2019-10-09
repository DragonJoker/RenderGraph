#pragma once

#include <RenderGraph/RenderGraphPrerequisites.hpp>

#include <iostream>
#include <stdexcept>
#include <fstream>

namespace test
{
	std::string getExecutableDirectory();
	crg::ImageData createImage( VkFormat format
		, uint32_t mipLevels = 1u );
	crg::ImageViewData createView( crg::ImageData image
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u );
	crg::ImageViewData createView( crg::ImageId image
		, VkFormat format
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u );

	template< typename TypeT >
	crg::Id< TypeT > makeId( TypeT const & data )
	{
		return { 0u, nullptr };
	}

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

		explicit inline LogStreambuf( std::string const & name
			, std::ostream & stream )
			: m_stream{ stream }
			, m_fstream{ getExecutableDirectory() + name + ".log" }
		{
			m_old = m_stream.rdbuf( this );
		}

		inline ~LogStreambuf()
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

	struct MessageData
	{
		std::string target;
		std::string error;
		std::string message;
		std::string function;
		int line;
	};

	struct TestCounts
	{
		void initialise( std::string const & name );
		void cleanup();

		std::string suiteName;
		std::string testName;
		uint32_t totalCount = 0u;
		uint32_t errorCount = 0u;

	private:
		std::unique_ptr< std::streambuf > tclog;
		std::unique_ptr< std::streambuf > tcout;
		std::unique_ptr< std::streambuf > tcerr;
	};

	class Exception
		: public std::exception
	{
	public:
		explicit Exception( MessageData const & data )
			: data{ data }
		{
		}

		inline const char * what()const noexcept override
		{
			return data.message.c_str();
		}

		MessageData data;
	};

	int reportTestSuite( TestCounts const & testCounts );
	void reportFailure( TestCounts & testCounts
		, MessageData const & data );
	void reportUnhandled( TestCounts & testCounts
		, MessageData const & data );
	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RenderGraph & value );
	void display( TestCounts & testCounts
		, crg::RenderGraph & graph );

	inline MessageData makeMessageData( std::string const & target
		, std::string error
		, std::string message
		, std::string function
		, int line )
	{
		return MessageData
		{
			target,
			error,
			message,
			function,
			line,
		};
	}

#define testStringify( x )\
	#x

#define testConcat2( x, y )\
	testStringify( x ) testStringify( y )

#define testConcat3( x, y, z )\
	testConcat2( x, y ) testStringify( z )

#define testConcat4( x, y, z, w )\
	testConcat3( x, y, z ) testStringify( w )

#define testSuiteBeginEx2( name, testCounts )\
		testCounts.initialise( name );\
		try\
		{\

#define testSuiteBeginEx( name, testCounts )\
	int result;\
	{\
		testSuiteBeginEx2( name, testCounts )

#define testSuiteBegin( name )\
	int result;\
	{\
		test::TestCounts testCounts;\
		testSuiteBeginEx2( name, testCounts )

#define testSuiteEnd()\
		}\
		catch ( std::exception & exc )\
		{\
			test::reportUnhandled( testCounts\
				, test::makeMessageData( testCounts.suiteName\
					, "GLOBAL"\
					, exc.what()\
					, __FUNCTION__\
					, __LINE__ ) );\
		}\
		catch ( ... )\
		{\
			test::reportUnhandled( testCounts\
				, test::makeMessageData( testCounts.suiteName\
					, "GLOBAL"\
					, "Unknown"\
					, __FUNCTION__\
					, __LINE__ ) );\
		}\
		result = test::reportTestSuite( testCounts );\
		testCounts.cleanup();\
	}\
	return result;

#define testBegin( name )\
	testCounts.testName = ( name );\
	auto testName = testCounts.testName;\
	std::cout << "********************************************************************************" << std::endl;\
	std::cout << "TEST: " << testCounts.testName << std::endl;\
	std::cout << "********************************************************************************" << std::endl;\
	try\
	{\

#define testEnd()\
	}\
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "TEST"\
				, exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "TEST"\
				, "Unknown"\
				, __FUNCTION__\
				, __LINE__ ) );\
	}

#define failure( x )\
	++testCounts.totalCount;\
	test::reportFailure( testCounts\
		, test::makeMessageData( testCounts.testName\
			, "FAILURE"\
			, #x\
			, __FUNCTION__\
			, __LINE__ ) );

#define require( x )\
	try\
	{\
		++testCounts.totalCount;\
		if ( !( x ) )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "COND"\
				, std::string{ #x }\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
			, "COND"\
			, std::string{ #x }\
			, __FUNCTION__\
			, __LINE__ ) );\
	}

#define check( x )\
	try\
	{\
		++testCounts.totalCount;\
		if ( !( x ) )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "REQCOND"\
				, std::string{ #x }\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
			, "REQCOND"\
			, std::string{ #x }\
			, __FUNCTION__\
			, __LINE__ ) );\
	}

#define checkEqual( x, y )\
	try\
	{\
		++testCounts.totalCount;\
		if ( ( x ) != ( y ) )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ #x } + " )\nRHS(\n" + std::string{ #y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ #x } + " )\nRHS(\n" + std::string{ #y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) );\
	}

#define checkEqualStr( x, y )\
	try\
	{\
		++testCounts.totalCount;\
		if ( ( x ) != ( y ) )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) );\
	}

#define checkNotEqual( x, y )\
	try\
	{\
		++testCounts.totalCount;\
		if ( ( x ) == ( y ) )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "NEQUAL"\
				, "\nLHS(\n" + std::string{ #x } + " )\nRHS(\n" + std::string{ #y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "NEQUAL"\
				, "\nLHS(\n" + std::string{ #x } + " )\nRHS(\n" + std::string{ #y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) );\
	}

#define checkNotEqualStr( x, y )\
	try\
	{\
		++testCounts.totalCount;\
		if ( ( x ) != ( y ) )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "NEQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "NEQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )"\
				, __FUNCTION__\
				, __LINE__ ) );\
	}

#define checkThrow( x )\
	try\
	{\
		++testCounts.totalCount;\
		( x ); \
		throw test::Exception{ test::makeMessageData( testCounts.testName\
			, "THROW"\
			, std::string{ #x }\
			, __FUNCTION__\
			, __LINE__ ) };\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( std::exception & )\
	{\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "THROW"\
				, std::string{ #x }\
				, __FUNCTION__\
				, __LINE__ ) );\
	}

#define checkNoThrow( x )\
	try\
	{\
		++testCounts.totalCount;\
		( x ); \
	}\
	catch ( std::exception & exc )\
	{\
		test::reportFailure( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "NOTHROW"\
				, std::string{ #x } + " " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
	}\
	catch ( ... )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "NOTHROW"\
				, std::string{ #x }\
				, __FUNCTION__\
				, __LINE__ ) );\
	}
}
