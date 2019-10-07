#include "Common.hpp"

#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ImageViewData.hpp>
#include <RenderGraph/RenderGraph.hpp>

#if defined( _WIN32 )
#	include <Windows.h>
#elif defined( __linux__ )
#	include <unistd.h>
#	include <dirent.h>
#	include <pwd.h>
#endif

namespace test
{
#if defined( _WIN32 )
	static char constexpr PathSeparator = '\\';
#elif defined( __linux__ )
	static char constexpr PathSeparator = '/';
#endif

	namespace
	{
		std::string getPath( std::string const & path )
		{
			return path.substr( 0, path.find_last_of( PathSeparator ) );
		}
	}

	crg::ImageData createImage( VkFormat format
		, uint32_t mipLevels )
	{
		crg::ImageData result{};
		result.format = format;
		result.mipLevels = mipLevels;
		result.extent = { 1024, 1024 };
		result.imageType = VK_IMAGE_TYPE_2D;
		result.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT;
		return result;
	}

	crg::ImageViewData createView( crg::ImageData image
		, uint32_t baseMipLevel
		, uint32_t levelCount )
	{
		crg::ImageViewData result{};
		result.format = image.format;
		result.viewType = VK_IMAGE_VIEW_TYPE_2D;
		result.subresourceRange.baseMipLevel = baseMipLevel;
		result.subresourceRange.levelCount = levelCount;
		result.subresourceRange.baseArrayLayer = 0u;
		result.subresourceRange.layerCount = 1u;
		return result;
	}

	crg::ImageViewData createView( crg::ImageId image
		, VkFormat format
		, uint32_t baseMipLevel
		, uint32_t levelCount )
	{
		crg::ImageViewData result{};
		result.image = image;
		result.format = format;
		result.viewType = VK_IMAGE_VIEW_TYPE_2D;
		result.subresourceRange.baseMipLevel = baseMipLevel;
		result.subresourceRange.levelCount = levelCount;
		result.subresourceRange.baseArrayLayer = 0u;
		result.subresourceRange.layerCount = 1u;
		return result;
	}

	std::ostream & operator<<( std::ostream & stream
		, std::vector< crg::ImageViewId > const & values )
	{
		std::string sep;

		for ( auto & value : values )
		{
			stream << sep << value.id;
			sep = ", ";
		}

		return stream;
	}

	std::ostream & operator<<( std::ostream & stream
		, crg::RenderPass const & value )
	{
		stream << value.name;
		return stream;
	}

	std::ostream & displayDebug( std::ostream & stream
		, std::set< crg::RenderGraphNode const * > & visited
		, uint32_t index
		, std::string prefix
		, crg::RenderGraphNode const & value )
	{
		stream << "\n" << prefix << "(" << index << ") " << ( *value.pass );
		visited.insert( &value );

		for ( auto & node : value.next )
		{
			stream << "\n  " << prefix << "(" << index + 1u << ") " << ( *node->pass );
			auto duplicate = visited.end() != visited.find( node );

			if ( !duplicate )
			{
				displayDebug( stream, visited, index + 1, "  " + prefix, *node );
			}
		}

		return stream;
	}

	void displayDebug( std::ostream & stream
		, crg::RenderGraph const & value )
	{
		std::set< crg::RenderGraphNode const * > visited;

		for ( auto & node : value.getGraph().next )
		{
			displayDebug( stream, visited, 1u, "  |-", *node );
		}
	}

	std::ostream & displayDot( std::ostream & stream
		, std::set< crg::RenderGraphNode const * > & visited
		, crg::RenderGraphNode const & value )
	{
		visited.insert( &value );

		for ( auto & node : value.next )
		{
			stream << "    " << ( *value.pass ) << " -> " << ( *node->pass ) << "[ label=\"";
			std::string sep;

			for ( auto & attach : node->attachesToPrev )
			{
				if ( attach.srcPass == value.pass )
				{
					for ( auto & dep : attach.dependencies )
					{
						stream << sep << dep.name;
						sep = "\\n";
					}
				}
			}

			stream << "\" ];\n";
			auto duplicate = visited.end() != visited.find( node );

			if ( !duplicate )
			{
				displayDot( stream, visited, *node );
			}
		}

		return stream;
	}

	void displayDot( std::ostream & stream
		, crg::RenderGraph const & value )
	{
		std::set< crg::RenderGraphNode const * > visited;
		std::string sep;
		stream << "digraph render {\n";

		for ( auto & node : value.getGraph().next )
		{
			displayDot( stream, visited, *node );
		}

		stream << "}\n";
	}

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RenderGraph const & value )
	{
		displayDot( stream, value );

		std::ofstream file{ testCounts.testName + ".dot" };
		displayDot( file, value );
	}

	void display( TestCounts & testCounts
		, crg::RenderGraph const & value )
	{
		display( testCounts, std::cout, value );
	}

#if defined( _WIN32 )

	std::string getExecutableDirectory()
	{
		std::string result;
		char path[FILENAME_MAX];
		DWORD res = ::GetModuleFileNameA( nullptr
			, path
			, sizeof( path ) );

		if ( res != 0 )
		{
			result = path;
		}

		result = getPath( result ) + PathSeparator;
		return result;
	}

#elif defined( __linux__ )

	std::string getExecutableDirectory()
	{
		std::string result;
		char path[FILENAME_MAX];
		char buffer[32];
		sprintf( buffer, "/proc/%d/exe", getpid() );
		int bytes = std::min< std::size_t >( readlink( buffer
			, path
			, sizeof( path ) )
			, sizeof( path ) - 1 );

		if ( bytes > 0 )
		{
			path[bytes] = '\0';
			result = path;
		}

		result = getPath( result ) + PathSeparator;
		return result;
	}

#endif

	void TestCounts::initialise( std::string const & name )
	{
		tclog = std::make_unique< test::LogStreambuf< test::DebugLogStreambufTraits > >( name, std::clog );
		tcout = std::make_unique< test::LogStreambuf< test::InfoLogStreambufTraits > >( name, std::cout );
		tcerr = std::make_unique< test::LogStreambuf< test::ErrorLogStreambufTraits > >( name, std::cerr );
	}

	void TestCounts::cleanup()
	{
		tclog.reset();
		tcout.reset();
		tcerr.reset();
	}

	int reportTestSuite( TestCounts const & testCounts )
	{
		int result;

		if ( testCounts.errorCount )
		{
			std::cout << "********************************************************************************" << std::endl;
			std::cout << "Test suite ended with some failures." << std::endl;
			std::cout << "Total checks count: " << testCounts.totalCount << std::endl;
			std::cout << "Failed checks count: " << testCounts.errorCount << std::endl;
			result = EXIT_FAILURE;
		}
		else
		{
			std::cout << "********************************************************************************" << std::endl;
			std::cout << "Test suite ended cleanly." << std::endl;
			std::cout << "Total checks count: " << testCounts.totalCount << std::endl;
			result = EXIT_SUCCESS;
		}

		return result;
	}

	void reportFailure( TestCounts & testCounts
		, MessageData const & data )
	{
		std::cout << "In " << data.function << ":" << data.line << ":" << std::endl;
		std::cout << "Failure for " << data.target << "( " << data.error << "( " << data.message << " ) )" << std::endl;
		++testCounts.errorCount;
	}

	void reportUnhandled( TestCounts & testCounts
		, MessageData const & data )
	{
		std::cout << "In " << data.function << ":" << data.line << ":" << std::endl;
		std::cout << "Unhandled Exception for " << data.target << "( " << data.error << "( " << data.message << " ) )" << std::endl;
		++testCounts.errorCount;
	}
}
