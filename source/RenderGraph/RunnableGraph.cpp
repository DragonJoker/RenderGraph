/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/DotExport.hpp"
#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/ResourceHandler.hpp"

#include <array>
#include <fstream>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace crg
{
	namespace
	{
		void display( RunnableGraph & value )
		{
			{
				std::ofstream file{ value.getGraph()->getName() + "_transitions.dot" };
				dot::displayTransitions( file, value, true );
			}
			{
				std::ofstream file{ value.getGraph()->getName() + "_passes.dot" };
				dot::displayPasses( file, value, true );
			}
		}

		size_t makeHash( LayoutState const & state )
		{
			auto result = std::hash< uint32_t >{}( state.layout );
			result = hashCombine( result, state.access );
			result = hashCombine( result, state.pipelineStage );
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

		VkImageSubresourceRange adaptRange( GraphContext & context
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

		bool isInRange( uint32_t lhsLeft
			, uint32_t lhsRight
			, uint32_t rhsLeft
			, uint32_t rhsRight )
		{
			return lhsLeft >= rhsLeft
				&& lhsRight <= rhsRight;
		}

		bool isInRange( VkImageSubresourceRange const & lhs
			, VkImageSubresourceRange const & rhs )
		{
			return isInRange( lhs.baseMipLevel
				, lhs.baseMipLevel + lhs.levelCount
				, rhs.baseMipLevel
				, rhs.baseMipLevel + rhs.levelCount )
				&& isInRange( lhs.baseArrayLayer
					, lhs.baseArrayLayer + lhs.layerCount
					, rhs.baseArrayLayer
					, rhs.baseArrayLayer + lhs.layerCount );
		}
	}

	VkImageAspectFlags getAspectMask( VkFormat format )noexcept
	{
		return ( isDepthStencilFormat( format )
			? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
			: ( isDepthFormat( format )
				? VK_IMAGE_ASPECT_DEPTH_BIT
				: ( isStencilFormat( format )
					? VK_IMAGE_ASPECT_STENCIL_BIT
					: VK_IMAGE_ASPECT_COLOR_BIT ) ) );
	}

	VkAccessFlags getAccessMask( VkImageLayout layout )noexcept
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

	VkPipelineStageFlags getStageMask( VkImageLayout layout )noexcept
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

	RunnableGraph::RunnableGraph( FrameGraph & graph
		, FramePassDependencies inputTransitions
		, FramePassDependencies outputTransitions
		, AttachmentTransitions transitions
		, GraphNodePtrArray nodes
		, RootNode rootNode
		, GraphContext & context )
		: m_graph{ graph }
		, m_context{ context }
		, m_inputTransitions{ std::move( inputTransitions ) }
		, m_outputTransitions{ std::move( outputTransitions ) }
		, m_transitions{ std::move( transitions ) }
		, m_nodes{ std::move( nodes ) }
		, m_rootNode{ std::move( rootNode ) }
	{
		display( *this );
		LayoutStateMap images;
		AccessStateMap buffers;

		for ( auto & pass : graph.m_passes )
		{
			doRegisterImages( *pass, images );
			doRegisterBuffers( *pass, buffers );
		}

		doCreateImages();
		doCreateImageViews();

		for ( auto & node : m_nodes )
		{
			if ( node->getKind() == GraphNode::Kind::FramePass )
			{
				auto & renderPassNode = nodeCast< FramePassNode >( *node );
				m_passes.push_back( renderPassNode.getFramePass().createRunnable( m_context
					, *this ) );
				auto & pass = m_passes.back();
				auto passCount = pass->getMaxPassCount();
				m_passesLayouts.emplace( &pass->getPass()
					, RemainingPasses{ passCount, {}, {} } );
				m_maxPassCount *= passCount;
			}
		}

		for ( uint32_t index = 0; index < m_maxPassCount; ++index )
		{
			m_viewsLayouts.push_back( images );
			m_buffersLayouts.push_back( buffers );
		}

		auto remainingCount = m_maxPassCount;
		uint32_t index = 0u;

		for ( auto & node : m_nodes )
		{
			auto it = m_passesLayouts.find( getFramePass( *node ) );
			remainingCount /= it->second.count;
			it->second.count = remainingCount;

			for ( uint32_t i = 0u; i < m_passes[index]->getMaxPassCount(); ++i )
			{
				it->second.views.push_back( { m_viewsLayouts.begin() + ( i * remainingCount )
					, m_viewsLayouts.begin() + ( ( i + 1 ) * remainingCount ) } );
				it->second.buffers.push_back( { m_buffersLayouts.begin() + ( i * remainingCount )
					, m_buffersLayouts.begin() + ( ( i + 1 ) * remainingCount ) } );
			}

			++index;
		}

		for ( auto & pass : m_passes )
		{
			pass->initialise();
		}
	}

	RunnableGraph::~RunnableGraph()
	{
	}

	void RunnableGraph::record()
	{
		for ( auto & pass : m_passes )
		{
			pass->record();
		}
	}

	VkImage RunnableGraph::createImage( ImageId const & image )
	{
		auto result = m_graph.m_handler.createImage( m_context, image );
		m_images[image] = result;
		return result;
	}

	VkImageView RunnableGraph::createImageView( ImageViewId const & view )
	{
		auto result = m_graph.m_handler.createImageView( m_context, view );
		m_imageViews[view] = result;
		return result;
	}

	SemaphoreWait RunnableGraph::run( VkQueue queue )
	{
		return run( SemaphoreWaitArray{}
			, queue );
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
		createImageView( result );
		return result;
	}

	VertexBuffer const & RunnableGraph::createQuadTriVertexBuffer( bool texCoords
		, Texcoord const & config )
	{
		return m_graph.getHandler().createQuadTriVertexBuffer( m_context
			, texCoords
			, config );
	}

	VkSampler RunnableGraph::createSampler( SamplerDesc const & samplerDesc )
	{
		return m_graph.getHandler().createSampler( m_context, samplerDesc );
	}

	LayoutState RunnableGraph::getCurrentLayout( FramePass const & pass
		, uint32_t passIndex
		, ImageViewId view )const
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.views.size() >= passIndex );
		auto & viewsLayouts = *it->second.views[passIndex].begin;
		auto imageIt = viewsLayouts.find( view.data->image.id );

		if ( imageIt != viewsLayouts.end() )
		{
			return doGetSubresourceRangeLayout( imageIt->second
				, getVirtualRange( view.data->image
					, view.data->info.viewType
					, view.data->info.subresourceRange ) );
		}

		return { VK_IMAGE_LAYOUT_UNDEFINED, 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	}

	LayoutState RunnableGraph::updateCurrentLayout( FramePass const & pass
		, uint32_t passIndex
		, ImageViewId view
		, LayoutState newLayout )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.views.size() >= passIndex );
		auto subresourceRange = getVirtualRange( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange );
		auto & views = it->second.views[passIndex];
		std::for_each( views.begin
			, views.end
			, [this, &subresourceRange, &newLayout, &view]( LayoutStateMap & viewsLayouts )
			{
				auto ires = viewsLayouts.emplace( view.data->image.id, LayerLayoutStates{} );
				doAddSubresourceRangeLayout( ires.first->second
					, subresourceRange
					, newLayout );
			} );
		return newLayout;
	}

	LayoutState RunnableGraph::getOutputLayout( FramePass const & pass
		, ImageViewId view
		, bool isCompute )const
	{
		LayoutState result{ VK_IMAGE_LAYOUT_UNDEFINED, 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		auto passIt = std::find_if( m_outputTransitions.begin()
			, m_outputTransitions.end()
			, [&pass]( FramePassTransitions const & lookup )
			{
				return lookup.pass == &pass;
			} );
		assert( passIt != m_outputTransitions.end() );
		auto it = std::find_if( passIt->transitions.viewTransitions.begin()
			, passIt->transitions.viewTransitions.end()
			, [&view]( ViewTransition const & lookup )
			{
				return match( *view.data, *lookup.data.data )
					|| view.data->source.end() != std::find_if( view.data->source.begin()
						, view.data->source.end()
						, [&lookup]( ImageViewId const & lookupView )
						{
							return match( *lookup.data.data, *lookupView.data );
						} )
					|| lookup.data.data->source.end() != std::find_if( lookup.data.data->source.begin()
						, lookup.data.data->source.end()
						, [&view]( ImageViewId const & lookupView )
						{
							return match( *view.data, *lookupView.data );
						} );
			} );

		if ( it != passIt->transitions.viewTransitions.end() )
		{
			result.layout = it->inputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
			result.access = it->inputAttach.getAccessMask();
			result.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
		}
		else
		{
			result = m_graph.getFinalLayout( view );

			if ( result.layout == VK_IMAGE_LAYOUT_UNDEFINED )
			{
				passIt = std::find_if( m_inputTransitions.begin()
					, m_inputTransitions.end()
					, [&pass]( FramePassTransitions const & lookup )
					{
						return lookup.pass == &pass;
					} );
				assert( passIt != m_inputTransitions.end() );
				auto it = std::find_if( passIt->transitions.viewTransitions.begin()
					, passIt->transitions.viewTransitions.end()
					, [&view]( ViewTransition const & lookup )
					{
						return match( *view.data, *lookup.data.data )
							|| view.data->source.end() != std::find_if( view.data->source.begin()
								, view.data->source.end()
								, [&lookup]( ImageViewId const & lookupView )
								{
									return match( *lookup.data.data, *lookupView.data );
								} )
							|| lookup.data.data->source.end() != std::find_if( lookup.data.data->source.begin()
								, lookup.data.data->source.end()
								, [&view]( ImageViewId const & lookupView )
								{
									return match( *view.data, *lookupView.data );
								} );
					} );

				if ( it == passIt->transitions.viewTransitions.end() )
				{
					result.layout = VK_IMAGE_LAYOUT_UNDEFINED;
					result.access = 0u;
					result.pipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				}
				else if ( it->inputAttach.getFlags() != 0u )
				{
					result.layout = it->inputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
					result.access = it->inputAttach.getAccessMask();
					result.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
				}
				else if ( it->outputAttach.isColourClearingAttach()
					|| it->outputAttach.isDepthClearingAttach() )
				{
					result.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					result.access = VK_ACCESS_SHADER_READ_BIT;
					result.pipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else
				{
					result.layout = it->outputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
					result.access = it->outputAttach.getAccessMask();
					result.pipelineStage = it->outputAttach.getPipelineStageFlags( isCompute );
				}
			}
		}

		return result;
	}

	AccessState RunnableGraph::getCurrentAccessState( FramePass const & pass
		, uint32_t passIndex
		, Buffer const & buffer )const
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.buffers.size() >= passIndex );
		auto & buffersLayouts = *it->second.buffers[passIndex].begin;
		auto bufferIt = buffersLayouts.find( buffer.buffer );

		if ( bufferIt != buffersLayouts.end() )
		{
			return bufferIt->second;
		}

		return { 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	}

	AccessState RunnableGraph::updateCurrentAccessState( FramePass const & pass
		, uint32_t passIndex
		, Buffer const & buffer
		, AccessState newState )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.buffers.size() >= passIndex );
		auto & buffers = it->second.buffers[passIndex];
		std::for_each( buffers.begin
			, buffers.end
			, [this, &newState, &buffer]( AccessStateMap & buffersLayouts )
			{
				buffersLayouts[buffer.buffer] = newState;
			} );
		return newState;
	}

	AccessState RunnableGraph::getOutputAccessState( FramePass const & pass
		, Buffer const & buffer
		, bool isCompute )const
	{
		AccessState result{ 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		auto passIt = std::find_if( m_outputTransitions.begin()
			, m_outputTransitions.end()
			, [&pass]( FramePassTransitions const & lookup )
			{
				return lookup.pass == &pass;
			} );
		assert( passIt != m_outputTransitions.end() );
		auto it = std::find_if( passIt->transitions.bufferTransitions.begin()
			, passIt->transitions.bufferTransitions.end()
			, [&buffer]( BufferTransition const & lookup )
			{
				return buffer == lookup.data;
			} );

		if ( it != passIt->transitions.bufferTransitions.end() )
		{
			result.access = it->inputAttach.getAccessMask();
			result.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
		}
		else
		{
			result = m_graph.getFinalAccessState( buffer );

			if ( result.access == 0u )
			{
				passIt = std::find_if( m_inputTransitions.begin()
					, m_inputTransitions.end()
					, [&pass]( FramePassTransitions const & lookup )
					{
						return lookup.pass == &pass;
					} );
				assert( passIt != m_inputTransitions.end() );
				auto it = std::find_if( passIt->transitions.bufferTransitions.begin()
					, passIt->transitions.bufferTransitions.end()
					, [&buffer]( BufferTransition const & lookup )
					{
						return buffer == lookup.data;
					} );

				if ( it == passIt->transitions.bufferTransitions.end() )
				{
					result.access = 0u;
					result.pipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				}
				else if ( it->inputAttach.getFlags() != 0u )
				{
					result.access = it->inputAttach.getAccessMask();
					result.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
				}
				else
				{
					result.access = it->outputAttach.getAccessMask();
					result.pipelineStage = it->outputAttach.getPipelineStageFlags( isCompute );
				}
			}
		}

		return result;
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageSubresourceRange const & subresourceRange
		, VkImageLayout currentLayout
		, VkImageLayout wantedLayout
		, VkAccessFlags currentMask
		, VkAccessFlags wantedMask
		, VkPipelineStageFlags previousStage
		, VkPipelineStageFlags nextStage )
	{
		if ( !m_context.device )
		{
			return;
		}

		if ( currentLayout != wantedLayout )
		{
			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
				, nullptr
				, currentMask
				, wantedMask
				, currentLayout
				, wantedLayout
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, createImage( image )
				, adaptRange( m_context
					, image.data->info.format
					, subresourceRange ) };
			m_context.vkCmdPipelineBarrier( commandBuffer
				, previousStage
				, nextStage
				, VK_DEPENDENCY_BY_REGION_BIT
				, 0u
				, nullptr
				, 0u
				, nullptr
				, 1u
				, &barrier );
		}
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState const & currentState
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, image
			, subresourceRange
			, currentState.layout
			, wantedState.layout
			, currentState.access
			, wantedState.access
			, currentState.pipelineStage
			, wantedState.pipelineStage );
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, VkImageLayout currentLayout
		, VkImageLayout wantedLayout
		, VkAccessFlags currentMask
		, VkAccessFlags wantedMask
		, VkPipelineStageFlags previousStage
		, VkPipelineStageFlags nextStage )
	{
		memoryBarrier( commandBuffer
			, view.data->image
			, view.data->info.subresourceRange
			, currentLayout
			, wantedLayout
			, currentMask
			, wantedMask
			, previousStage
			, nextStage );
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, LayoutState const & currentState
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, view
			, currentState.layout
			, wantedState.layout
			, currentState.access
			, wantedState.access
			, currentState.pipelineStage
			, wantedState.pipelineStage );
	}

	void RunnableGraph::imageMemoryBarrier( VkCommandBuffer commandBuffer
		, Attachment const & from
		, uint32_t fromIndex
		, Attachment const & to
		, uint32_t toIndex
		, bool isCompute )
	{
		assert( from.isImage()
			&& to.isImage()
			&& from.view( fromIndex ) == to.view( toIndex ) );
		memoryBarrier( commandBuffer
			, from.view( fromIndex )
			, from.getImageLayout( m_context.separateDepthStencilLayouts )
			, to.getImageLayout( m_context.separateDepthStencilLayouts )
			, from.getAccessMask()
			, to.getAccessMask()
			, from.getPipelineStageFlags( isCompute )
			, to.getPipelineStageFlags( isCompute ) );
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, Buffer const & buffer
		, VkDeviceSize offset
		, VkDeviceSize range
		, VkAccessFlags currentMask
		, VkAccessFlags wantedMask
		, VkPipelineStageFlags previousStage
		, VkPipelineStageFlags nextStage )
	{
		if ( !m_context.device )
		{
			return;
		}

		if ( currentMask != wantedMask )
		{
			VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
				, nullptr
				, currentMask
				, wantedMask
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, buffer.buffer
				, offset
				, range };
			m_context.vkCmdPipelineBarrier( commandBuffer
				, previousStage
				, nextStage
				, VK_DEPENDENCY_BY_REGION_BIT
				, 0u
				, nullptr
				, 1u
				, &barrier
				, 0u
				, nullptr );
		}
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, Buffer const & buffer
		, VkDeviceSize offset
		, VkDeviceSize range
		, AccessState const & currentState
		, AccessState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, buffer
			, offset
			, range
			, currentState.access
			, wantedState.access
			, currentState.pipelineStage
			, wantedState.pipelineStage );
	}

	void RunnableGraph::bufferMemoryBarrier( VkCommandBuffer commandBuffer
		, Attachment const & from
		, Attachment const & to
		, bool isCompute )
	{
		assert( from.isBuffer()
			&& to.isBuffer()
			&& from.buffer.buffer == to.buffer.buffer
			&& from.buffer.offset == to.buffer.offset
			&& from.buffer.range == to.buffer.range );
		auto previousMask = from.getAccessMask();
		auto nextMask = to.getAccessMask();
		auto previousStage = from.getPipelineStageFlags( isCompute );
		auto nextStage = to.getPipelineStageFlags( isCompute );
		memoryBarrier( commandBuffer
			, from.buffer.buffer
			, from.buffer.offset
			, from.buffer.range
			, from.getAccessMask()
			, to.getAccessMask()
			, from.getPipelineStageFlags( isCompute )
			, to.getPipelineStageFlags( isCompute ) );
	}

	void RunnableGraph::doRegisterImages( FramePass const & pass
		, LayoutStateMap & images )
	{
		static LayoutState const defaultState{ VK_IMAGE_LAYOUT_UNDEFINED
			, getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
			, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };

		for ( auto & attach : pass.images )
		{
			if ( attach.count > 1u )
			{
				for ( uint32_t i = 0u; i < attach.count; ++i )
				{
					auto view = attach.view( i );
					auto image = view.data->image;
					createImage( image );
					createImageView( view );
					auto ires = images.emplace( image.id, LayerLayoutStates{} );

					if ( ires.second )
					{
						auto & layers = ires.first->second;
						auto sliceArrayCount = ( image.data->info.extent.depth > 1u
							? image.data->info.extent.depth
							: image.data->info.arrayLayers );

						for ( uint32_t slice = 0; slice < sliceArrayCount; ++slice )
						{
							auto & levels = layers.emplace( slice, MipLayoutStates{} ).first->second;

							for ( uint32_t level = 0; level < image.data->info.mipLevels; ++level )
							{
								levels.emplace( level, defaultState );
							}
						}
					}
				}
			}
			else
			{
				auto view = attach.view();
				auto image = view.data->image;
				createImage( image );
				createImageView( view );
				auto ires = images.emplace( image.id, LayerLayoutStates{} );

				if ( ires.second )
				{
					auto & layers = ires.first->second;
					auto sliceArrayCount = ( image.data->info.extent.depth > 1u
						? image.data->info.extent.depth
						: image.data->info.arrayLayers );

					for ( uint32_t slice = 0; slice < sliceArrayCount; ++slice )
					{
						auto & levels = layers.emplace( slice, MipLayoutStates{} ).first->second;

						for ( uint32_t level = 0; level < image.data->info.mipLevels; ++level )
						{
							levels.emplace( level, defaultState );
						}
					}
				}
			}
		}
	}

	void RunnableGraph::doRegisterBuffers( FramePass const & pass
		, AccessStateMap & buffers )
	{
		static AccessState const defaultState{ getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
			, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };

		for ( auto & attach : pass.buffers )
		{
			auto buffer = attach.buffer.buffer.buffer;
			buffers.emplace( buffer, defaultState );
		}
	}

	void RunnableGraph::doCreateImages()
	{
		if ( !m_context.device )
		{
			return;
		}

		for ( auto & img : m_graph.m_images )
		{
			createImage( img );
		}
	}

	void RunnableGraph::doCreateImageViews()
	{
		for ( auto & view : m_graph.m_imageViews )
		{
			createImageView( view );
			m_imageViews[view] = m_graph.m_handler.createImageView( m_context, view );
		}
	}

	LayoutState RunnableGraph::doGetSubresourceRangeLayout( LayerLayoutStates const & ranges
		, VkImageSubresourceRange const & range )const
	{
		std::unordered_map< size_t, LayoutState > states;

		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto & layers = ranges.find( range.baseArrayLayer + layerIdx )->second;

			for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
			{
				auto state = layers.find( range.baseMipLevel + levelIdx )->second;
				states.emplace( makeHash( state ), state );
			}
		}

		if ( states.size() == 1u )
		{
			return states.begin()->second;
		}

		return { VK_IMAGE_LAYOUT_UNDEFINED
			, getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
			, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };
	}

	LayoutState RunnableGraph::doAddSubresourceRangeLayout( LayerLayoutStates & ranges
		, VkImageSubresourceRange const & range
		, LayoutState const & newLayout )
	{
		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto & layers = ranges.find( range.baseArrayLayer + layerIdx )->second;

			for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
			{
				auto & level = layers.find( range.baseMipLevel + levelIdx )->second;
				level.layout = newLayout.layout;
				level.access = newLayout.access;
				level.pipelineStage = newLayout.pipelineStage;
			}
		}

		return newLayout;
	}
}
