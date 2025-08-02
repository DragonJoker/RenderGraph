#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <RenderGraph/Exception.hpp>

#include <gtest/gtest.h>

namespace test
{
	std::string getExecutableDirectory();

	struct TestCounts
	{
		explicit TestCounts( std::string const & testName )
			: testName{ testName }
		{
		}

		std::string diffLines( std::string const & check
			, std::string const & ref );

		std::string testName;
	};

	class Exception
		: public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class TestSuite
		: public ::testing::Environment
	{
	public:
		TestSuite( std::string const & name );

	private:
		std::unique_ptr< std::streambuf > tclog;
		std::unique_ptr< std::streambuf > tcout;
		std::unique_ptr< std::streambuf > tcerr;
	};

	int testsMain( int argc, char ** argv, std::string_view testSuiteName );

	std::string sortLines( std::string const & value );

#define testStringify( x )\
	#x

#define nameConcat( X, Y ) nameConcat_( X, Y )
#define nameConcat_( X, Y ) X ## Y

#define testBegin( name )\
	test::TestCounts testCounts{ name };

#define testEnd()

#define require( x )\
	try\
	{\
		if ( !( x ) )\
		{\
			throw test::Exception{ std::string{ testStringify( x ) } };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		GTEST_FATAL_FAILURE_( ( std::string{ testStringify( x )" failed." } + exc.what() ).c_str() );\
	}\
	catch ( ... )\
	{\
		GTEST_FATAL_FAILURE_( "Unknown unhandled exception." );\
	}

#define check( x )\
	EXPECT_TRUE( x );

#define checkEqual( x, y )\
	EXPECT_EQ( x, y );

#define checkEqualSortedLines( x, y )\
	try\
	{\
		auto diff = testCounts.diffLines( test::sortLines( x ), test::sortLines( y ) );\
		if ( !diff.empty() )\
		{\
			throw test::Exception{ "LHS(\n" + std::string{ x } + " )\nRHS(\n" + std::string{ y } + " )\nDIFF(\n" + diff + " )" };\
		}\
	}\
	catch ( test::Exception & exc )\
	{\
		GTEST_FATAL_FAILURE_( ( std::string{ #x" failed." } + exc.what() ).c_str() );\
	}\
	catch ( crg::Exception & exc )\
	{\
		GTEST_FATAL_FAILURE_( ( std::string{ #x" failed." } + exc.what() ).c_str() );\
	}\
	catch ( ... )\
	{\
		GTEST_FATAL_FAILURE_( "Unknown unhandled exception." );\
	}

#define checkThrow( x, excType )\
	EXPECT_THROW( x, excType );

#define checkNoThrow( x )\
	EXPECT_NO_THROW( x );

#define testSuiteMain()\
	int main( int argc, char ** argv )\
	{\
		return test::testsMain( argc, argv, CRG_TestSuiteNameString );\
	}
}
