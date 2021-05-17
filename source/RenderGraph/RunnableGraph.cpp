/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/GraphVisitor.hpp"

#include <string>

namespace crg
{
	namespace
	{
		VkImageCreateInfo convert( ImageData const & data )
		{
			return VkImageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
				, nullptr
				, data.flags
				, data.imageType
				, data.format
				, { data.extent.width, data.extent.height, 1u }
				, data.mipLevels
				, data.arrayLayers
				, data.samples
				, data.tiling
				, data.usage
				, VK_SHARING_MODE_EXCLUSIVE
				, 0u
				, nullptr
				, VK_IMAGE_LAYOUT_UNDEFINED
			};
		}

		VkImageViewCreateInfo convert( ImageViewData const & data
			, ImageMemoryMap const & images )
		{
			auto it = images.find( data.image );
			assert( it != images.end() );
			return VkImageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
				, nullptr
				, data.flags
				, it->second.first
				, data.viewType
				, data.format
				, { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
				, data.subresourceRange
			};
		}

		class DfsVisitor
			: public GraphVisitor
		{
		public:
			static GraphAdjacentNodeArray submit( crg::GraphAdjacentNode node )
			{
				GraphAdjacentNodeArray result;
				std::set< crg::GraphNode const * > visited;
				DfsVisitor vis{ result, visited };
				node->accept( &vis );
				return result;
			}

		private:
			DfsVisitor( GraphAdjacentNodeArray & result
				, std::set< crg::GraphNode const * > & visited )
				: m_result{ result }
				, m_visited{ visited }
			{
			}

			void visitRootNode( crg::RootNode * node )override
			{
				m_result.push_back( node );

				for ( auto & next : node->getNext() )
				{
					next->accept( this );
				}
			}

			void visitRenderPassNode( crg::RenderPassNode * node )override
			{
				m_result.push_back( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					if ( m_visited.end() == m_visited.find( next ) )
					{
						next->accept( this );
					}
				}
			}

		private:
			GraphAdjacentNodeArray & m_result;
			std::set< crg::GraphNode const * > & m_visited;
		};
	}

	RunnableGraph::RunnableGraph( RenderGraph graph
		, GraphContext context )
		: m_graph{ std::move( graph ) }
		, m_context{ std::move( context ) }
	{
		m_graph.compile();
		doCreateImages();
		doCreateImageViews();
		auto dfsNodes = DfsVisitor::submit( m_graph.getGraph() );

		for ( auto & node : dfsNodes )
		{
			if ( node->getKind() == GraphNode::Kind::RenderPass )
			{
				auto & renderPassNode = nodeCast< RenderPassNode >( *node );
				m_passes.push_back( renderPassNode.getRenderPass().createRunnable( m_context , *this ) );
			}
		}
	}

	RunnableGraph::~RunnableGraph()
	{
		for ( auto & view : m_imageViews )
		{
			crgUnregisterObject( m_context, view.second );
			m_context.vkDestroyImageView( m_context.device, view.second, m_context.allocator );
		}

		for ( auto & img : m_images )
		{
			crgUnregisterObject( m_context, img.second.second );
			m_context.vkFreeMemory( m_context.device, img.second.second, m_context.allocator );
			crgUnregisterObject( m_context, img.second.first );
			m_context.vkDestroyImage( m_context.device, img.second.first, m_context.allocator );
		}
	}

	VkSemaphore RunnableGraph::run( VkSemaphore toWait )
	{
		auto result = toWait;

		//for ( auto & pass : m_passes )
		//{
		//	result = pass->run( result );
		//}

		return result;
	}

	VkImageView RunnableGraph::getImageView( Attachment const & attach )const
	{
		auto it = m_graph.m_attachViews.find( attach.viewData.name );

		if ( it == m_graph.m_attachViews.end()
			|| *it->second.data != attach.viewData )
		{
			return VK_NULL_HANDLE;
		}

		auto viewIt = m_imageViews.find( it->second );
		assert( viewIt != m_imageViews.end() );
		return viewIt->second;
	}

	void RunnableGraph::doCreateImages()
	{
		for ( auto & img : m_graph.m_images )
		{
			// Create image
			VkImage image;
			auto createInfo = convert( *img.first.data );
			auto res = m_context.vkCreateImage( m_context.device
				, &createInfo
				, m_context.allocator
				, &image );
			checkVkResult( res, "Image creation" );
			crgRegisterObject( m_context, img.first.data->name, image );

			// Create Image memory
			VkMemoryRequirements requirements{};
			m_context.vkGetImageMemoryRequirements( m_context.device
				, image
				, &requirements );
			uint32_t deduced = m_context.deduceMemoryType( requirements.memoryTypeBits
				, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			VkDeviceMemory memory;
			res = m_context.vkAllocateMemory( m_context.device
				, &allocateInfo
				, m_context.allocator
				, &memory );
			checkVkResult( res, "Image memory allocation" );
			crgRegisterObject( m_context, img.first.data->name, memory );

			// Bind image and memory
			res = m_context.vkBindImageMemory( m_context.device
				, image
				, memory
				, 0u );
			checkVkResult( res, "Image memory binding" );
			m_images[img.first] = { image, memory };
		}
	}

	void RunnableGraph::doCreateImageViews()
	{
		for ( auto & view : m_graph.m_imageViews )
		{
			VkImageView imageView;
			auto createInfo = convert( *view.first.data, m_images );
			auto res = m_context.vkCreateImageView( m_context.device
				, &createInfo
				, m_context.allocator
				, &imageView );
			checkVkResult( res, "ImageView creation" );
			crgRegisterObject( m_context, view.first.data->name, imageView );
			m_imageViews[view.first] = imageView;
		}
	}
}
