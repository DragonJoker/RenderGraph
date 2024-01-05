#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace test
{
	std::string getExecutableDirectory();

	struct TestCounts
	{
		void initialise( std::string const & suiteName );
		void cleanup();
		std::string diffLines( std::string const & check
			, std::string const & ref );
		void updateName( std::string const & testName );

		std::string suiteName;
		std::string testName;
		uint32_t testId = 0u;
		uint32_t totalCount = 0u;
		uint32_t errorCount = 0u;

	private:
		std::unique_ptr< std::streambuf > tclog;
		std::unique_ptr< std::streambuf > tcout;
		std::unique_ptr< std::streambuf > tcerr;
	};

	struct MessageData
	{
		std::string target;
		std::string error;
		std::string message;
		std::string function;
		int line;
	};

	inline MessageData makeMessageData( std::string target
		, std::string error
		, std::string message
		, std::string function
		, int line )
	{
		return MessageData{ std::move( target )
			, std::move( error )
			, std::move( message )
			, std::move( function )
			, line };
	}

	class Exception
		: public std::exception
	{
	public:
		explicit Exception( MessageData data )
			: data{ std::move( data ) }
		{
		}

		inline const char * what()const noexcept override
		{
			return data.message.c_str();
		}

		MessageData data;
	};

	int reportTestSuite( TestCounts const & testCounts );
	void reportFailure( TestCounts & testCounts, MessageData const & data );
	void reportUnhandled( TestCounts & testCounts, MessageData const & data );
	std::string sortLines( std::string const & value );

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
	testCounts.updateName( name );\
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
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "REQCOND"\
				, std::string{ #x } + " " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ #x } + " )\nRHS(\n" + std::string{ #y } + " ) " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " ) " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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

#define checkEqualLines( x, y )\
	try\
	{\
		++testCounts.totalCount;\
		auto diff = testCounts.diffLines( x, y );\
		if ( !diff.empty() )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )\nDIFF(\n" + diff + " )"\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " ) " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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

#define checkEqualSortedLines( x, y )\
	try\
	{\
		++testCounts.totalCount;\
		auto diff = testCounts.diffLines( test::sortLines( x ), test::sortLines( y ) );\
		if ( !diff.empty() )\
		{\
			throw test::Exception{ test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )\nDIFF(\n" + diff + " )"\
				, __FUNCTION__\
				, __LINE__ ) };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		test::reportFailure( testCounts, exc.data );\
	}\
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "EQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " ) " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "NEQUAL"\
				, "\nLHS(\n" + std::string{ #x } + " )\nRHS(\n" + std::string{ #y } + " ) " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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
	catch ( std::exception & exc )\
	{\
		test::reportUnhandled( testCounts\
			, test::makeMessageData( testCounts.testName\
				, "NEQUAL"\
				, "\nLHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " ) " + exc.what()\
				, __FUNCTION__\
				, __LINE__ ) );\
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
