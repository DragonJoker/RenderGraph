/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/DotExport.hpp"
#include "RenderGraph/GraphVisitor.hpp"

#include <array>
#include <fstream>
#include <string>
#include <type_traits>

namespace crg
{
	namespace
	{

		void display( FrameGraph & value )
		{
			{
				std::ofstream file{ value.getGraph()->getName() + "_transitions.dot" };
				dot::displayTransitions( file, value );
			}
			{
				std::ofstream file{ value.getGraph()->getName() + "_passes.dot" };
				dot::displayPasses( file, value );
			}
		}

		struct Quad
		{
			using Data = std::array< float, 2u >;
			struct Vertex
			{
				Data position;
				Data texture;
			};

			Vertex vertex[6];
		};

		size_t makeHash( bool texCoords
			, bool invertU
			, bool invertV )
		{
			return ( ( texCoords ? 0x01 : 0x00 ) << 0 )
				| ( ( invertU ? 0x01 : 0x00 ) << 1 )
				| ( ( invertV ? 0x01 : 0x00 ) << 2 );
		}

		template< typename T >
		inline size_t hashCombine( size_t hash
			, T const & rhs )
		{
			const uint64_t kMul = 0x9ddfea08eb382d69ULL;
			auto seed = hash;

			std::hash< T > hasher;
			uint64_t a = ( hasher( rhs ) ^ seed ) * kMul;
			a ^= ( a >> 47 );

			uint64_t b = ( seed ^ a ) * kMul;
			b ^= ( b >> 47 );

			hash = static_cast< std::size_t >( b * kMul );
			return hash;
		}

		size_t makeHash( SamplerDesc const & samplerDesc )
		{
			auto result = std::hash< uint32_t >{}( samplerDesc.magFilter );
			result = hashCombine( result, samplerDesc.minFilter );
			result = hashCombine( result, samplerDesc.mipmapMode );
			result = hashCombine( result, samplerDesc.addressModeU );
			result = hashCombine( result, samplerDesc.addressModeV );
			result = hashCombine( result, samplerDesc.addressModeW );
			result = hashCombine( result, samplerDesc.mipLodBias );
			result = hashCombine( result, samplerDesc.minLod );
			result = hashCombine( result, samplerDesc.maxLod );
			return result;
		}

		std::string getName( VkFilter filter )
		{
			switch ( filter )
			{
			case VK_FILTER_NEAREST:
				return "Nearest";
			case VK_FILTER_LINEAR:
				return "Linear";
			case VK_FILTER_CUBIC_IMG:
				return "Cubic";
			default:
				return "Unknown";
			}
		}

		VkImageCreateInfo convert( ImageData const & data )
		{
			return data.info;
		}

		VkImageViewCreateInfo convert( ImageViewData const & data
			, ImageMemoryMap const & images )
		{
			auto it = images.find( data.image );
			assert( it != images.end() );
			auto result = data.info;
			result.image = it->second.first;
			return result;
		}

		VkAccessFlags getAccessMask( VkImageLayout layout )
		{
			VkAccessFlags result{ 0u };

			switch ( layout )
			{
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				result |= VK_ACCESS_MEMORY_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				result |= VK_ACCESS_SHADER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				result |= VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				result |= VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
#ifdef VK_NV_shading_rate_image
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
				result |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;
				break;
#endif
#ifdef VK_EXT_fragment_density_map
			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
				result |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
				break;
#endif
			default:
				break;
			}

			return result;
		}

		VkPipelineStageFlags getStageMask( VkImageLayout layout )
		{
			VkPipelineStageFlags result{ 0u };

			switch ( layout )
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				result |= VK_PIPELINE_STAGE_HOST_BIT;
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
				result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
#ifdef VK_EXT_fragment_density_map
			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
#endif
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
#ifdef VK_NV_shading_rate_image
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
				result |= VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
				break;
#endif
			default:
				break;
			}

			return result;
		}

		VkImageSubresourceRange adaptRange( GraphContext const & context
			, VkFormat format
			, VkImageSubresourceRange const & subresourceRange )
		{
			VkImageSubresourceRange result = subresourceRange;

			if ( !context.separateDepthStencilLayouts )
			{
				if ( isDepthStencilFormat( format )
					&& ( result.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT
						|| result.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT ) )
				{
					result.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			return result;
		}

		class DfsVisitor
			: public GraphVisitor
		{
		public:
			static ConstGraphAdjacentNodeArray submit( ConstGraphAdjacentNode node )
			{
				ConstGraphAdjacentNodeArray result;
				std::set< ConstGraphAdjacentNode > visited;
				DfsVisitor vis{ result, visited };
				node->accept( &vis );
				return result;
			}

		private:
			DfsVisitor( ConstGraphAdjacentNodeArray & result
				, std::set< ConstGraphAdjacentNode > & visited )
				: m_result{ result }
				, m_visited{ visited }
			{
			}

			void visitRootNode( RootNode const * node )override
			{
				m_result.push_back( node );

				for ( auto & next : node->getNext() )
				{
					next->accept( this );
				}
			}

			void visitFramePassNode( FramePassNode const * node )override
			{
				m_result.push_back( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					if ( m_visited.end() == m_visited.find( next )
						&& getHitCount( node, next ) == 0 )
					{
						m_visited.insert( next );
						next->accept( this );
						auto & rhs = *getFramePass( *next );
						auto pendingIt = m_pending.find( &rhs );

						if ( pendingIt != m_pending.end() )
						{
							auto node = pendingIt->second;
							m_pending.erase( pendingIt );
							node->accept( this );
						}
					}
				}
			}

		private:
			size_t getHitCount( FramePassNode const * lhsNode
				, GraphAdjacentNode rhsNode )
			{
				auto & lhs = lhsNode->getFramePass();
				auto & rhs = *getFramePass( *rhsNode );
				auto ires = m_hitCount.emplace( &rhs, 0u );

				if ( ires.second )
				{
					ires.first->second = rhs.depends;
				}

				auto it = std::find( ires.first->second.begin()
					, ires.first->second.end()
					, &lhs );

				if ( it != ires.first->second.end() )
				{
					ires.first->second.erase( it );
					auto pendingIt = m_pending.find( &lhs );

					if ( pendingIt != m_pending.end() )
					{
						m_pending.erase( pendingIt );
					}
				}
				else
				{
					m_pending.emplace( ires.first->second.front(), rhsNode );
				}

				return ires.first->second.size();
			}

		private:
			ConstGraphAdjacentNodeArray & m_result;
			std::set< ConstGraphAdjacentNode > & m_visited;
			std::unordered_map< FramePass const *, FramePassArray > m_hitCount;
			std::unordered_map< FramePass const *, GraphAdjacentNode > m_pending;
		};
	}

	RunnableGraph::RunnableGraph( FrameGraph graph
		, GraphContext context )
		: m_graph{ std::move( graph ) }
		, m_context{ std::move( context ) }
	{
		m_graph.compile();
		display( m_graph );
		doCreateImages();
		doCreateImageViews();
		auto dfsNodes = DfsVisitor::submit( m_graph.getGraph() );

		for ( auto & node : dfsNodes )
		{
			if ( node->getKind() == GraphNode::Kind::FramePass )
			{
				auto & renderPassNode = nodeCast< FramePassNode >( *node );
				m_passes.push_back( renderPassNode.getFramePass().createRunnable( m_context
					, *this ) );
			}
		}

		for ( auto & pass : m_passes )
		{
			m_maxPassCount = std::max( m_maxPassCount
				, pass->initialise() );
		}

		for ( uint32_t index = 1; index < m_maxPassCount; ++index )
		{
			for ( auto & pass : m_passes )
			{
				pass->initialise( index );
			}
		}
	}

	RunnableGraph::~RunnableGraph()
	{
		for ( auto & vertexBuffer : m_vertexBuffers )
		{
			if ( vertexBuffer.second->memory )
			{
				crgUnregisterObject( m_context, vertexBuffer.second->memory );
				m_context.vkFreeMemory( m_context.device
					, vertexBuffer.second->memory
					, m_context.allocator );
			}

			if ( vertexBuffer.second->buffer )
			{
				crgUnregisterObject( m_context, vertexBuffer.second->buffer );
				m_context.vkDestroyBuffer( m_context.device
					, vertexBuffer.second->buffer
					, m_context.allocator );
			}
		}

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

		for ( auto & sampler : m_samplers )
		{
			crgUnregisterObject( m_context, sampler.second );
			m_context.vkDestroySampler( m_context.device
				, sampler.second
				, m_context.allocator );
		}
	}

	void RunnableGraph::record()
	{
		for ( auto & pass : m_passes )
		{
			pass->record();
		}
	}

	void RunnableGraph::recordInto( VkCommandBuffer commandBuffer )
	{
		for ( auto & pass : m_passes )
		{
			pass->recordInto( commandBuffer );
		}
	}

	SemaphoreWait RunnableGraph::run( SemaphoreWait toWait
		, VkQueue queue )
	{
		return run( ( toWait.semaphore
				? SemaphoreWaitArray{ 1u, toWait }
				: SemaphoreWaitArray{} )
			, queue );
	}

	SemaphoreWait RunnableGraph::run( SemaphoreWaitArray const & toWait
		, VkQueue queue )
	{
		auto result = toWait;

		for ( auto & pass : m_passes )
		{
			result = { 1u, pass->run( result, queue ) };
		}

		return result.front();
	}

	ImageViewId RunnableGraph::createView( ImageViewData const & view )
	{
		auto result = m_graph.createView( view );
		doCreateImageView( result );
		return result;
	}

	VkImage RunnableGraph::getImage( ImageId const & image )const
	{
		auto it = m_images.find( image );

		if ( it == m_images.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second.first;
	}

	VkImage RunnableGraph::getImage( ImageViewId const & view )const
	{
		return getImage( view.data->image );
	}

	VkImage RunnableGraph::getImage( Attachment const & attach
		, uint32_t index )const
	{
		return getImage( attach.view( index ) );
	}

	VkImageView RunnableGraph::getImageView( ImageViewId const & view )const
	{
		auto it = m_imageViews.find( view );

		if ( it == m_imageViews.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second;
	}

	VkImageView RunnableGraph::getImageView( Attachment const & attach
		, uint32_t index )const
	{
		return getImageView( attach.view( index ) );
	}

	VertexBuffer const & RunnableGraph::createQuadVertexBuffer( bool texCoords
		, bool invertU
		, bool invertV )
	{
		auto hash = makeHash( texCoords, invertU, invertV );
		auto ires = m_vertexBuffers.emplace( hash, std::make_unique< VertexBuffer >() );

		if ( ires.second )
		{
			auto & vertexBuffer = ires.first->second;
			VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
				, nullptr
				, 0u
				, 4u * sizeof( Quad::Vertex )
				, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				, VK_SHARING_MODE_EXCLUSIVE
				, 0u
				, nullptr };
			auto res = m_context.vkCreateBuffer( m_context.device
				, &createInfo
				, m_context.allocator
				, &vertexBuffer->buffer );
			checkVkResult( res, "Vertex buffer creation" );
			crgRegisterObject( m_context, "QuadVertexBuffer", vertexBuffer->buffer );

			VkMemoryRequirements requirements{};
			m_context.vkGetBufferMemoryRequirements( m_context.device
				, vertexBuffer->buffer
				, &requirements );
			uint32_t deduced = m_context.deduceMemoryType( requirements.memoryTypeBits
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			res = m_context.vkAllocateMemory( m_context.device
				, &allocateInfo
				, m_context.allocator
				, &vertexBuffer->memory );
			checkVkResult( res, "Buffer memory allocation" );
			crgRegisterObject( m_context, "QuadVertexMemory", vertexBuffer->memory );

			res = m_context.vkBindBufferMemory( m_context.device
				, vertexBuffer->buffer
				, vertexBuffer->memory
				, 0u );
			checkVkResult( res, "Buffer memory binding" );

			Quad::Vertex * buffer{};
			res = m_context.vkMapMemory( m_context.device
				, vertexBuffer->memory
				, 0u
				, VK_WHOLE_SIZE
				, 0u
				, reinterpret_cast< void ** >( &buffer ) );
			checkVkResult( res, "Buffer memory mapping" );

			if ( buffer )
			{
				std::array< Quad::Vertex, 4u > vertexData{ Quad::Vertex{ { -1.0, -1.0 }
					, ( texCoords
						? Quad::Data{ ( invertU ? 1.0f : 0.0f ), ( invertV ? 1.0f : 0.0f ) }
						: Quad::Data{ 0.0f, 0.0f } ) }
					, Quad::Vertex{ { -1.0, +1.0 }
						, ( texCoords
							? Quad::Data{ ( invertU ? 1.0f : 0.0f ), ( invertV ? 0.0f : 1.0f ) }
							: Quad::Data{ 0.0f, 0.0f } ) }
					, Quad::Vertex{ { +1.0f, -1.0f }
						, ( texCoords
							? Quad::Data{ ( invertU ? 0.0f : 1.0f ), ( invertV ? 1.0f : 0.0f ) }
							: Quad::Data{ 0.0f, 0.0f } ) }
					, Quad::Vertex{ { +1.0f, +1.0f }
						, ( texCoords
							? Quad::Data{ ( invertU ? 0.0f : 1.0f ), ( invertV ? 0.0f : 1.0f ) }
							: Quad::Data{ 0.0f, 0.0f } ) } };
				std::copy( vertexData.begin(), vertexData.end(), buffer );

				VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
					, 0u
					, vertexBuffer->memory
					, 0u
					, VK_WHOLE_SIZE };
				m_context.vkFlushMappedMemoryRanges( m_context.device, 1u, &memoryRange );
				m_context.vkUnmapMemory( m_context.device, vertexBuffer->memory );
			}

			vertexBuffer->vertexAttribs.push_back( { 0u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, position ) } );

			if ( texCoords )
			{
				vertexBuffer->vertexAttribs.push_back( { 1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, texture ) } );
			}

			vertexBuffer->vertexBindings.push_back( { 0u, sizeof( Quad::Vertex ), VK_VERTEX_INPUT_RATE_VERTEX } );
			vertexBuffer->inputState = VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( vertexBuffer->vertexBindings.size() )
				, vertexBuffer->vertexBindings.data()
				, uint32_t( vertexBuffer->vertexAttribs.size() )
				, vertexBuffer->vertexAttribs.data() };
		}

		return *ires.first->second;
	}

	VkSampler RunnableGraph::createSampler( SamplerDesc const & samplerDesc )
	{
		auto ires = m_samplers.emplace( makeHash( samplerDesc ), VkSampler{} );

		if ( ires.second )
		{
			VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
				, nullptr
				, 0u
				, samplerDesc.magFilter
				, samplerDesc.minFilter
				, samplerDesc.mipmapMode
				, samplerDesc.addressModeU
				, samplerDesc.addressModeV
				, samplerDesc.addressModeW
				, samplerDesc.mipLodBias // mipLodBias
				, VK_FALSE // anisotropyEnable
				, 0.0f // maxAnisotropy
				, VK_FALSE // compareEnable
				, VK_COMPARE_OP_ALWAYS // compareOp
				, samplerDesc.minLod
				, samplerDesc.maxLod
				, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
				, VK_FALSE };
			auto res = m_context.vkCreateSampler( m_context.device
				, &createInfo
				, m_context.allocator
				, &ires.first->second );
			checkVkResult( res, "Sampler creation" );
			crgRegisterObject( m_context, "Sampler" + std::to_string( makeHash( samplerDesc ) ), ires.first->second );
		}

		return ires.first->second;
	}

	VkImageLayout RunnableGraph::getCurrentLayout( ImageViewId view )const
	{
		auto it = m_viewsLayouts.find( view.id );

		if ( it != m_viewsLayouts.end() )
		{
			return it->second;
		}

		for ( auto & source : view.data->source )
		{
			it = m_viewsLayouts.find( source.id );

			if ( it != m_viewsLayouts.end() )
			{
				return it->second;
			}
		}

		return VK_IMAGE_LAYOUT_UNDEFINED;
	}

	VkImageLayout RunnableGraph::updateCurrentLayout( ImageViewId view
		, VkImageLayout newLayout )
	{
		m_viewsLayouts[view.id] = newLayout;
		return newLayout;
	}

	VkImageLayout RunnableGraph::getOutputLayout( crg::FramePass const & pass
		, ImageViewId view )const
	{
		VkImageLayout result{ VK_IMAGE_LAYOUT_UNDEFINED };
		auto passIt = m_graph.getOutputTransitions().find( &pass );
		assert( passIt != m_graph.getOutputTransitions().end() );
		auto it = std::find_if( passIt->second.begin()
			, passIt->second.end()
			, [&view]( AttachmentTransition const & lookup )
			{
				return view == lookup.view
					|| view.data->source.end() != std::find( view.data->source.begin()
						, view.data->source.end()
						, lookup.view )
					|| lookup.view.data->source.end() != std::find( lookup.view.data->source.begin()
						, lookup.view.data->source.end()
						, view );
			} );

		if ( it != passIt->second.end() )
		{
			result = it->inputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
		}
		else
		{
			passIt = m_graph.getInputTransitions().find( &pass );
			assert( passIt != m_graph.getInputTransitions().end() );
			auto it = std::find_if( passIt->second.begin()
				, passIt->second.end()
				, [&view]( AttachmentTransition const & lookup )
				{
					return view == lookup.view
						|| view.data->source.end() != std::find( view.data->source.begin()
							, view.data->source.end()
							, lookup.view )
						|| lookup.view.data->source.end() != std::find( lookup.view.data->source.begin()
							, lookup.view.data->source.end()
							, view );
				} );

			if ( it == passIt->second.end() )
			{
				result = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else if ( it->inputAttach.getFlags() != 0u )
			{
				result = it->inputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
			}
			else if ( it->outputAttach.isColourClearing()
				|| it->outputAttach.isDepthClearing() )
			{
				result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			else
			{
				result = it->outputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
			}
		}

		return result;
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, VkImageLayout currentLayout
		, VkImageLayout wantedLayout )
	{
		if ( currentLayout != wantedLayout )
		{
			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
				, nullptr
				, getAccessMask( currentLayout )
				, getAccessMask( wantedLayout )
				, currentLayout
				, wantedLayout
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, getImage( view.data->image )
				, adaptRange( m_context
					, view.data->info.format
					, view.data->info.subresourceRange ) };
			m_context.vkCmdPipelineBarrier( commandBuffer
				, getStageMask( currentLayout )
				, getStageMask( wantedLayout )
				, VK_DEPENDENCY_BY_REGION_BIT
				, 0u
				, nullptr
				, 0u
				, nullptr
				, 1u
				, &barrier );
		}
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

	void RunnableGraph::doCreateImageView( ImageViewId view )
	{
		auto ires = m_imageViews.emplace( view, VkImageView{} );

		if ( ires.second )
		{
			auto createInfo = convert( *view.data, m_images );
			auto res = m_context.vkCreateImageView( m_context.device
				, &createInfo
				, m_context.allocator
				, &ires.first->second );
			checkVkResult( res, "ImageView creation" );
			crgRegisterObject( m_context, view.data->name, ires.first->second );
		}
	}

	void RunnableGraph::doCreateImageViews()
	{
		for ( auto & view : m_graph.m_imageViews )
		{
			doCreateImageView( view.first );
		}
	}
}
