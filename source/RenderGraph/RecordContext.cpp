/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RecordContext.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/ResourceHandler.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>
#include <string>
#include <type_traits>
#include <unordered_set>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <fstream>
#pragma warning( pop )

namespace crg
{
	//************************************************************************************************

	namespace recctx
	{
		static VkImageSubresourceRange adaptRange( GraphContext const & context
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
	}

	//************************************************************************************************

	RecordContext::RecordContext( ContextResourcesCache & resources )
		: m_handler{ &resources.getHandler() }
		, m_resources{ &resources }
		, m_prevPipelineState{ 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
		, m_currPipelineState{ 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
		, m_nextPipelineState{ 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT }
	{
	}

	RecordContext::RecordContext( ResourceHandler & handler )
		: m_handler{ &handler }
		, m_resources{ nullptr }
	{
	}

	void RecordContext::addStates( RecordContext const & data )
	{
		m_images.addStates( data.m_images );

		for ( auto & state : data.m_buffers )
		{
			m_buffers.insert( state );
		}

		if ( m_prevPipelineState.access < data.m_currPipelineState.access )
		{
			m_prevPipelineState = data.m_currPipelineState;
		}
	}

	void RecordContext::setNextPipelineState( PipelineState const & state
		, LayerLayoutStatesMap const & imageLayouts )
	{
		m_prevPipelineState = m_currPipelineState;
		m_currPipelineState = m_nextPipelineState;
		m_nextPipelineState = state;
		m_nextImages = LayerLayoutStatesHandler{ imageLayouts };
	}

	void RecordContext::setLayoutState( crg::ImageViewId view
		, LayoutState const & layoutState )
	{
		m_images.setLayoutState( view, layoutState );
	}

	LayoutState RecordContext::getLayoutState( ImageViewId view )const
	{
		return m_images.getLayoutState( view );
	}

	void RecordContext::setLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState const & layoutState )
	{
		m_images.setLayoutState( image
			, viewType
			, subresourceRange
			, layoutState );
	}

	LayoutState RecordContext::getLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange )const
	{
		return m_images.getLayoutState( image
			, viewType
			, subresourceRange );
	}

	LayoutState RecordContext::getNextLayoutState( ImageViewId view )const
	{
		return m_nextImages.getLayoutState( view );
	}

	LayoutState RecordContext::getNextLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange )const
	{
		return m_nextImages.getLayoutState( image
			, viewType
			, subresourceRange );
	}

	void RecordContext::registerImplicitTransition( RunnablePass const & pass
			, crg::ImageViewId view
			, RecordContext::ImplicitAction action )
	{
		registerImplicitTransition( { &pass, view, std::move( action ) } );
	}

	void RecordContext::registerImplicitTransition( ImplicitTransition transition )
	{
		m_implicitTransitions.emplace_back( std::move( transition ) );
	}

	void RecordContext::runImplicitTransition( VkCommandBuffer commandBuffer
		, uint32_t index
		, crg::ImageViewId view )
	{
		auto it = std::find_if( m_implicitTransitions.begin()
			, m_implicitTransitions.end()
			, [&view]( ImplicitTransition const & lookup )
			{
				return lookup.view == view;
			} );

		if ( it != m_implicitTransitions.end() )
		{
			auto pass = it->pass;
			auto action = it->action;
			m_implicitTransitions.erase( it );

			if ( !pass->isEnabled() )
			{
				action( *this, commandBuffer, index );
			}
		}
	}

	void RecordContext::setAccessState( VkBuffer buffer
		, [[maybe_unused]] BufferSubresourceRange const & subresourceRange
		, AccessState const & layoutState )
	{
		m_buffers.insert_or_assign( buffer, layoutState );
	}

	AccessState const & RecordContext::getAccessState( VkBuffer buffer
		, [[maybe_unused]] BufferSubresourceRange const & subresourceRange )const
	{
		if ( auto bufferIt = m_buffers.find( buffer ); bufferIt != m_buffers.end() )
		{
			return bufferIt->second;
		}

		static AccessState const dummy{ 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		return dummy;
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, VkImageLayout initialLayout
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, initialLayout
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageSubresourceRange const & subresourceRange
		, VkImageLayout initialLayout
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, image
			, VkImageViewType( image.data->info.imageType )
			, subresourceRange
			, initialLayout
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, VkImageLayout initialLayout
		, LayoutState const & wantedState
		, bool force )
	{
		auto & resources = getResources();

		if( !resources->device )
		{
			return;
		}

		auto range = recctx::adaptRange( *m_resources
				, image.data->info.format
				, subresourceRange );
		auto from = getLayoutState( image
			, viewType
			, range );

		if ( from.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			from = { initialLayout
				, getAccessMask( initialLayout )
				, getStageMask( initialLayout ) };
		}

		if ( force
			|| ( ( from.layout != wantedState.layout
				|| from.state.pipelineStage != wantedState.state.pipelineStage )
				&& wantedState.layout != VK_IMAGE_LAYOUT_UNDEFINED ) )
		{
			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
				, nullptr
				, from.state.access
				, wantedState.state.access
				, from.layout
				, wantedState.layout
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, resources.createImage( image )
				, range };
			resources->vkCmdPipelineBarrier( commandBuffer
				, from.state.pipelineStage
				, wantedState.state.pipelineStage
				, VK_DEPENDENCY_BY_REGION_BIT
				, 0u
				, nullptr
				, 0u
				, nullptr
				, 1u
				, &barrier );
			setLayoutState( image
				, viewType
				, range
				, wantedState );
		}
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, VK_IMAGE_LAYOUT_UNDEFINED
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, image
			, VkImageViewType( image.data->info.imageType )
			, subresourceRange
			, VK_IMAGE_LAYOUT_UNDEFINED
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, image
			, viewType
			, subresourceRange
			, VK_IMAGE_LAYOUT_UNDEFINED
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, VkAccessFlags initialMask
		, VkPipelineStageFlags initialStage
		, AccessState const & wantedState
		, bool force )
	{
		auto const & resources = getResources();

		if ( !resources->device )
		{
			return;
		}

		auto from = getAccessState( buffer
			, subresourceRange );

		if ( from.pipelineStage == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT )
		{
			from = { initialMask, initialStage };
		}

		if ( force
			|| ( from.access != wantedState.access
				|| from.pipelineStage != wantedState.pipelineStage ) )
		{
			VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
				, nullptr
				, from.access
				, wantedState.access
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, buffer
				, subresourceRange.offset
				, subresourceRange.size };
			resources->vkCmdPipelineBarrier( commandBuffer
				, from.pipelineStage
				, wantedState.pipelineStage
				, VK_DEPENDENCY_BY_REGION_BIT
				, 0u
				, nullptr
				, 1u
				, &barrier
				, 0u
				, nullptr );
			setAccessState( buffer
				, subresourceRange
				, wantedState );
		}
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, AccessState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, buffer
			, subresourceRange
			, 0u
			, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
			, wantedState
			, force );
	}

	GraphContext & RecordContext::getContext()const
	{
		return getResources();
	}

	RecordContext::ImplicitAction RecordContext::copyImage( ImageViewId srcView
		, ImageViewId dstView
		, VkExtent2D extent )
	{
		return [srcView, dstView, extent]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			auto & resources = recContext.getResources();
			recContext.runImplicitTransition( commandBuffer
				, index
				, srcView );
			auto & srcSubresource = srcView.data->info.subresourceRange;
			auto & dstSubresource = dstView.data->info.subresourceRange;
			VkImageCopy region{ VkImageSubresourceLayers{ srcSubresource.aspectMask, srcSubresource.baseMipLevel, srcSubresource.baseArrayLayer, 1u }
				, VkOffset3D{ 0u, 0u, 0u }
				, VkImageSubresourceLayers{ dstSubresource.aspectMask, dstSubresource.baseMipLevel, dstSubresource.baseArrayLayer, 1u }
				, VkOffset3D{ 0u, 0u, 0u }
			, { extent.width, extent.height, 1u } };
			recContext.memoryBarrier( commandBuffer
				, srcView.data->image
				, srcView.data->info.viewType
				, srcView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ) );
			recContext.memoryBarrier( commandBuffer
				, dstView.data->image
				, dstView.data->info.viewType
				, dstView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );
			resources->vkCmdCopyImage( commandBuffer
				, resources.createImage( srcView.data->image )
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, resources.createImage( dstView.data->image )
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &region );
		};
	}

	RecordContext::ImplicitAction RecordContext::blitImage( ImageViewId srcView
		, ImageViewId dstView
		, VkOffset2D srcOffset
		, VkExtent2D srcExtent
		, VkOffset2D dstOffset
		, VkExtent2D dstExtent
		, VkFilter filter
		, VkImageLayout finalLayout )
	{
		return [srcView, dstView, srcOffset, srcExtent, dstOffset, dstExtent, filter, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			auto & resources = recContext.getResources();
			recContext.runImplicitTransition( commandBuffer
				, index
				, srcView );
			auto & srcSubresource = srcView.data->info.subresourceRange;
			auto & dstSubresource = dstView.data->info.subresourceRange;
			VkImageBlit region{ VkImageSubresourceLayers{ srcSubresource.aspectMask, srcSubresource.baseMipLevel, srcSubresource.baseArrayLayer, 1u }
				, { VkOffset3D{ srcOffset.x, srcOffset.y, 0u }
					, VkOffset3D{ int32_t( srcExtent.width ), int32_t( srcExtent.height ), 1 } }
				, VkImageSubresourceLayers{ dstSubresource.aspectMask, dstSubresource.baseMipLevel, dstSubresource.baseArrayLayer, 1u }
				, { VkOffset3D{ dstOffset.x, dstOffset.y, 0u }
					, VkOffset3D{ int32_t( dstExtent.width ), int32_t( dstExtent.height ), 1 } } };
			recContext.memoryBarrier( commandBuffer
				, srcView.data->image
				, srcView.data->info.viewType
				, srcView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ) );
			recContext.memoryBarrier( commandBuffer
				, dstView.data->image
				, dstView.data->info.viewType
				, dstView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );
			resources->vkCmdBlitImage( commandBuffer
				, resources.createImage( srcView.data->image )
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, resources.createImage( dstView.data->image )
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &region
				, filter );

			if ( finalLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				recContext.memoryBarrier( commandBuffer
					, dstView.data->image
					, dstView.data->info.viewType
					, dstView.data->info.subresourceRange
					, makeLayoutState( finalLayout ) );
			}
		};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( Attachment attach
		, VkImageLayout finalLayout )
	{
		return [attach, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			auto & resources = recContext.getResources();
			auto dstView = attach.view( index );
			recContext.memoryBarrier( commandBuffer
				, dstView.data->image
				, dstView.data->info.viewType
				, dstView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );

			if ( isColourFormat( getFormat( dstView ) ) )
			{
				auto colour = attach.getClearValue().color;
				resources->vkCmdClearColorImage( commandBuffer
					, resources.createImage( dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &colour
					, 1u
					, &dstView.data->info.subresourceRange );
			}
			else
			{
				auto depthStencil = attach.getClearValue().depthStencil;
				resources->vkCmdClearDepthStencilImage( commandBuffer
					, resources.createImage( dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &depthStencil
					, 1u
					, &dstView.data->info.subresourceRange );
			}

			if ( finalLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				recContext.memoryBarrier( commandBuffer
					, dstView.data->image
					, dstView.data->info.viewType
					, dstView.data->info.subresourceRange
					, makeLayoutState( finalLayout ) );
			}
		};
	}

	ContextResourcesCache & RecordContext::getResources()const
	{
		if ( !m_resources )
		{
			Logger::logError( "No resources available." );
			CRG_Exception( "No resources available." );
		}

		return *m_resources;
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( ImageViewId dstView
		, VkClearValue const & clearValue
		, VkImageLayout finalLayout )
	{
		return [clearValue, dstView, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, [[maybe_unused]] uint32_t index )
		{
			auto & resources = recContext.getResources();
			recContext.memoryBarrier( commandBuffer
				, dstView.data->image
				, dstView.data->info.viewType
				, dstView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );

			if ( isColourFormat( getFormat( dstView ) ) )
			{
				auto colour = clearValue.color;
				resources->vkCmdClearColorImage( commandBuffer
					, resources.createImage( dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &colour
					, 1u
					, &dstView.data->info.subresourceRange );
			}
			else
			{
				auto depthStencil = clearValue.depthStencil;
				resources->vkCmdClearDepthStencilImage( commandBuffer
					, resources.createImage( dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &depthStencil
					, 1u
					, &dstView.data->info.subresourceRange );
			}

			if ( finalLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				recContext.memoryBarrier( commandBuffer
					, dstView.data->image
					, dstView.data->info.viewType
					, dstView.data->info.subresourceRange
					, makeLayoutState( finalLayout ) );
			}
		};
	}

	//************************************************************************************************
}
