#include "Common.hpp"

#include <RenderGraph/GraphVisitor.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ImageViewData.hpp>
#include <RenderGraph/RenderGraph.hpp>

#include <functional>
#include <map>
#include <sstream>

namespace test
{
	namespace
	{
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
				, crg::GraphAdjacentNode node
				, std::set< crg::GraphNode const * > & visited )
			{
				DotOutVisitor vis{ stream, visited };
				node->accept( &vis );
			}

			static void submit( std::ostream & stream
				, crg::GraphAdjacentNode node )
			{
				std::set< crg::GraphNode const * > visited;
				submit( stream, node, visited );
			}

			static void submit( std::ostream & stream
				, crg::AttachmentTransitionArray const & transitions )
			{
				stream << "digraph \"Transitions\" {\n";
				stream << "  rankdir = \"LR\"";

				for ( auto & transition : transitions )
				{
					std::string name{ "Trans. to\\n" + transition.dstInput.attachment.name };
					stream << "    \"" << name << "\" [ shape=square ];\n";

					for ( auto & srcOutput : transition.srcOutputs )
					{
						for ( auto pass : srcOutput.passes )
						{
							stream << "    \"" << pass->name << "\" -> \"" << name << "\" [ label=\"" << srcOutput.attachment.name << "\" ];\n";
						}
					}

					for ( auto pass : transition.dstInput.passes )
					{
						stream << "    \"" << name << "\" -> \"" << pass->name << "\" [ label=\"" << transition.dstInput.attachment.name << "\" ];\n";
					}
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

			void printEdge( crg::GraphNode * lhs
				, crg::GraphNode * rhs )
			{
				std::string sep;
				auto transitions = rhs->getAttachsToPrev( lhs );
				std::sort( transitions.begin()
					, transitions.end()
					, []( crg::AttachmentTransition const & lhs, crg::AttachmentTransition const & rhs )
					{
						return lhs.srcOutputs.front().attachment.name < rhs.srcOutputs.front().attachment.name;
					} );
				uint32_t index{ 1u };

				for ( auto & transition : transitions )
				{
					auto & srcOutput = transition.srcOutputs.front();
					std::string name{ srcOutput.attachment.name + "\\nto\\n" + transition.dstInput.attachment.name };
					m_stream << "    \"" << name << "\" [ shape=square ];\n";
					m_stream << "    \"" << lhs->getName() << "\" -> \"" << name << "\" [ label=\"" << srcOutput.attachment.name << "\" ];\n";
					m_stream << "    \"" << name << "\" -> \"" << rhs->getName() << "\" [ label=\"" << transition.dstInput.attachment.name << "\" ];\n";
				}
			}

			void submit( crg::GraphNode * node )
			{
				submit( m_stream
					, node
					, m_visited );
			}

			void visitRootNode( crg::RootNode * node )override
			{
				m_stream << "digraph \"" << node->getName() << "\" {\n";

				for ( auto & next : node->getNext() )
				{
					submit( next );
				}

				m_stream << "}\n";
			}

			void visitRenderPassNode( crg::RenderPassNode * node )override
			{
				m_visited.insert( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					printEdge( node, next );

					if ( m_visited.end() == m_visited.find( next ) )
					{
						submit( next );
					}
				}
			}

		private:
			std::ostream & m_stream;
			std::set< crg::GraphNode const * > & m_visited;
		};

		void displayPasses( TestCounts & testCounts
			, std::ostream & stream
			, crg::RenderGraph & value )
		{
			DotOutVisitor::submit( stream, value.getGraph() );
			std::ofstream file{ testCounts.testName + ".dot" };
			DotOutVisitor::submit( file, value.getGraph() );
		}

		void displayTransitions( TestCounts & testCounts
			, std::ostream & stream
			, crg::RenderGraph & value )
		{
			DotOutVisitor::submit( stream, value.getTransitions() );
			std::ofstream file{ testCounts.testName + "_transitions.dot" };
			DotOutVisitor::submit( file, value.getTransitions() );
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

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RenderGraph & value )
	{
		std::stringstream trans;
		displayTransitions( testCounts, trans, value );
		displayPasses( testCounts, stream, value );
	}

	void display( TestCounts & testCounts
		, crg::RenderGraph & value )
	{
		display( testCounts, std::cout, value );
	}
}
