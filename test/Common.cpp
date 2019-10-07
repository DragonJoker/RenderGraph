#include "Common.hpp"

#include <RenderGraph/GraphVisitor.hpp>
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

	class DotOutVisitor
		: public crg::GraphVisitor
	{
	public:
		static void submit( std::ostream & stream
			, crg::GraphNode * node
			, std::set< crg::GraphNode const * > & visited )
		{
			DotOutVisitor vis{ stream, visited };
			node->accept( &vis );
		}

		static void submit( std::ostream & stream
			, crg::RootNode const & node )
		{
			std::set< crg::GraphNode const * > visited;
			stream << "digraph render {\n";

			for ( auto & node : node.next )
			{
				DotOutVisitor vis{ stream, visited };
				node->accept( &vis );
			}

			stream << "}\n";
		}

	private:
		DotOutVisitor( std::ostream & stream
			, std::set< crg::GraphNode const * > & visited )
			: m_stream{ stream }
			, m_visited{ visited }
		{
		}

		void submit( crg::GraphNode * node )
		{
			submit( m_stream
				, node
				, m_visited );
		}

		void visitRootNode( crg::RootNode * node )override
		{
			for ( auto & next : node->next )
			{
				submit( next );
			}
		}

		void visitRenderPassNode( crg::RenderPassNode * node )override
		{
			m_visited.insert( node );

			for ( auto & next : node->next )
			{
				m_stream << "    " << ( *node->pass ) << " -> " << ( *crg::nodeCast< crg::RenderPassNode >( *next ).pass ) << "[ label=\"";
				std::string sep;

				for ( auto & attach : crg::nodeCast< crg::RenderPassNode >( *next ).attachesToPrev )
				{
					if ( attach.srcPass == node->pass )
					{
						for ( auto & dep : attach.dependencies )
						{
							m_stream << sep << dep.name;
							sep = "\\n";
						}
					}
				}

				m_stream << "\" ];\n";
				auto duplicate = m_visited.end() != m_visited.find( next );

				if ( !duplicate )
				{
					submit( next );
				}
			}
		}

		void visitPhiNode( crg::PhiNode * node )override
		{
			for ( auto & next : node->next )
			{
				submit( next );
			}
		}

	private:
		std::ostream & m_stream;
		std::set< crg::GraphNode const * > & m_visited;
	};

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RenderGraph const & value )
	{
		DotOutVisitor::submit( stream, value.getGraph() );

		std::ofstream file{ testCounts.testName + ".dot" };
		DotOutVisitor::submit( file, value.getGraph() );
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
