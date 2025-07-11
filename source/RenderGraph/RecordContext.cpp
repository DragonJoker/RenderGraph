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
		static ImageSubresourceRange adaptRange( GraphContext const & context
			, PixelFormat format
			, ImageSubresourceRange const & subresourceRange )
		{
			ImageSubresourceRange result = subresourceRange;

			if ( !context.separateDepthStencilLayouts
				&& isDepthStencilFormat( format )
				&& ( checkFlag( result.aspectMask, ImageAspectFlags::eDepth )
					|| checkFlag( result.aspectMask, ImageAspectFlags::eStencil ) ) )
				result.aspectMask = ImageAspectFlags::eDepthStencil;

			return result;
		}

		static void clearAttachment( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, ImageViewId dstView
			, ClearColorValue const & clearValue
			, ImageLayout finalLayout )
		{
			auto & resources = recContext.getResources();
			recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
			auto subresourceRange = convert( getSubresourceRange( dstView ) );
			assert( isColourFormat( getFormat( dstView ) ) );
			auto vkClearValue = convert( clearValue );
			resources->vkCmdClearColorImage( commandBuffer
				, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, &vkClearValue, 1u, &subresourceRange );

			if ( finalLayout != ImageLayout::eUndefined )
				recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
		}

		static void clearAttachment( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, ImageViewId dstView
			, ClearDepthStencilValue const & clearValue
			, ImageLayout finalLayout )
		{
			auto & resources = recContext.getResources();
			recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
			auto subresourceRange = convert( getSubresourceRange( dstView ) );
			assert( isDepthOrStencilFormat( getFormat( dstView ) ) );
			auto vkClearValue = convert( clearValue );
			resources->vkCmdClearDepthStencilImage( commandBuffer
				, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, &vkClearValue, 1u, &subresourceRange );

			if ( finalLayout != ImageLayout::eUndefined )
				recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
		}
	}

	//************************************************************************************************

	RecordContext::RecordContext( ContextResourcesCache & resources )
		: m_handler{ &resources.getHandler() }
		, m_resources{ &resources }
		, m_prevPipelineState{ AccessFlags::eNone, PipelineStageFlags::eBottomOfPipe }
		, m_currPipelineState{ AccessFlags::eNone, PipelineStageFlags::eBottomOfPipe }
		, m_nextPipelineState{ AccessFlags::eNone, PipelineStageFlags::eBottomOfPipe }
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

	void RecordContext::setLayoutState( ImageViewId view
		, LayoutState const & layoutState )
	{
		m_images.setLayoutState( view, layoutState );
	}

	LayoutState RecordContext::getLayoutState( ImageViewId view )const
	{
		return m_images.getLayoutState( view );
	}

	void RecordContext::setLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange
		, LayoutState const & layoutState )
	{
		m_images.setLayoutState( image
			, viewType
			, subresourceRange
			, layoutState );
	}

	LayoutState RecordContext::getLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange )const
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
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange )const
	{
		return m_nextImages.getLayoutState( image
			, viewType
			, subresourceRange );
	}

	void RecordContext::registerImplicitTransition( RunnablePass const & pass
			, ImageViewId view
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
		, ImageViewId view )
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

		static AccessState const dummy{ AccessFlags::eNone, PipelineStageFlags::eBottomOfPipe };
		return dummy;
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, ImageLayout initialLayout
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
		, ImageSubresourceRange const & subresourceRange
		, ImageLayout initialLayout
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, image
			, ImageViewType( image.data->info.imageType )
			, subresourceRange
			, initialLayout
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange
		, ImageLayout initialLayout
		, LayoutState const & wantedState
		, bool force )
	{
		auto range = recctx::adaptRange( *m_resources
				, image.data->info.format
				, subresourceRange );
		auto from = getLayoutState( image
			, viewType
			, range );

		if ( from.layout == ImageLayout::eUndefined )
		{
			from = makeLayoutState( initialLayout );
		}

		if ( force
			|| ( ( from.layout != wantedState.layout
				|| from.state.pipelineStage != wantedState.state.pipelineStage )
				&& wantedState.layout != ImageLayout::eUndefined ) )
		{
			auto & resources = getResources();
			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
				, nullptr
				, getAccessFlags( from.state.access )
				, getAccessFlags( wantedState.state.access )
				, convert( from.layout )
				, convert( wantedState.layout )
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, resources.createImage( image )
				, convert( range ) };
			resources->vkCmdPipelineBarrier( commandBuffer
				, getPipelineStageFlags( from.state.pipelineStage )
				, getPipelineStageFlags( wantedState.state.pipelineStage )
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
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, ImageSubresourceRange const & subresourceRange
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, image
			, ImageViewType( image.data->info.imageType )
			, subresourceRange
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange
		, LayoutState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, image
			, viewType
			, subresourceRange
			, ImageLayout::eUndefined
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, AccessState const & initialState
		, AccessState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, buffer
			, subresourceRange
			, initialState.access
			, initialState.pipelineStage
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, AccessFlags initialMask
		, PipelineStageFlags initialStage
		, AccessState const & wantedState
		, bool force )
	{
		auto from = getAccessState( buffer
			, subresourceRange );

		if ( checkFlag( from.pipelineStage, PipelineStageFlags::eBottomOfPipe ) )
		{
			from = { initialMask, initialStage };
		}

		if ( force
			|| ( from.access != wantedState.access
				|| from.pipelineStage != wantedState.pipelineStage ) )
		{
			auto const & resources = getResources();
			VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
				, nullptr
				, getAccessFlags( from.access )
				, getAccessFlags( wantedState.access )
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, buffer
				, subresourceRange.offset
				, subresourceRange.size };
			resources->vkCmdPipelineBarrier( commandBuffer
				, getPipelineStageFlags( from.pipelineStage )
				, getPipelineStageFlags( wantedState.pipelineStage )
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
			, AccessFlags::eNone
			, PipelineStageFlags::eBottomOfPipe
			, wantedState
			, force );
	}

	GraphContext & RecordContext::getContext()const
	{
		return getResources();
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

	RecordContext::ImplicitAction RecordContext::copyImage( ImageViewId srcView
		, ImageViewId dstView
		, Extent2D const & extent
		, ImageLayout finalLayout )
	{
		return [srcView, dstView, extent, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			recContext.runImplicitTransition( commandBuffer, index, srcView );
			auto & srcSubresource = srcView.data->info.subresourceRange;
			auto & dstSubresource = dstView.data->info.subresourceRange;
			VkImageCopy region{ getSubresourceLayer( srcSubresource ), VkOffset3D{ 0u, 0u, 0u }
				, getSubresourceLayer( dstSubresource ), VkOffset3D{ 0u, 0u, 0u }
				, { extent.width, extent.height, 1u } };
			recContext.memoryBarrier( commandBuffer, srcView, makeLayoutState( ImageLayout::eTransferSrc ) );
			recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
			auto & resources = recContext.getResources();
			resources->vkCmdCopyImage( commandBuffer
				, resources.createImage( srcView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u, &region );

			if ( finalLayout != ImageLayout::eUndefined )
				recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
		};
	}

	RecordContext::ImplicitAction RecordContext::blitImage( ImageViewId srcView
		, ImageViewId dstView
		, Rect2D const & srcRect
		, Rect2D const & dstRect
		, FilterMode filter
		, ImageLayout finalLayout )
	{
		return [srcView, dstView, srcRect, dstRect, filter, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			recContext.runImplicitTransition( commandBuffer, index, srcView );
			auto & srcSubresource = srcView.data->info.subresourceRange;
			auto & dstSubresource = dstView.data->info.subresourceRange;
			VkImageBlit region{ getSubresourceLayer( srcSubresource ), { VkOffset3D{ srcRect.offset.x, srcRect.offset.y, 0u }, VkOffset3D{ int32_t( srcRect.extent.width ), int32_t( srcRect.extent.height ), 1 } }
				, getSubresourceLayer( dstSubresource ), { VkOffset3D{ dstRect.offset.x, dstRect.offset.y, 0u }, VkOffset3D{ int32_t( dstRect.extent.width ), int32_t( dstRect.extent.height ), 1 } } };
			recContext.memoryBarrier( commandBuffer, srcView, makeLayoutState( ImageLayout::eTransferSrc ) );
			recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
			auto & resources = recContext.getResources();
			resources->vkCmdBlitImage( commandBuffer
				, resources.createImage( srcView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u, &region, convert( filter ) );

			if ( finalLayout != ImageLayout::eUndefined )
				recContext.memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
		};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( Attachment const & attach
		, ImageLayout finalLayout )
	{
		return [attach, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			auto dstView = attach.view( index );
			if ( isColourFormat( getFormat( dstView ) ) )
				recctx::clearAttachment( recContext, commandBuffer, dstView, getClearColorValue( attach.getClearValue() ), finalLayout );
			else
				recctx::clearAttachment( recContext, commandBuffer, dstView, getClearDepthStencilValue( attach.getClearValue() ), finalLayout );
		};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( ImageViewId dstView
		, ClearColorValue const & clearValue
		, ImageLayout finalLayout )
	{
		return [clearValue, dstView, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, [[maybe_unused]] uint32_t index )
			{
				recctx::clearAttachment( recContext, commandBuffer, dstView, clearValue, finalLayout );
			};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( ImageViewId dstView
		, ClearDepthStencilValue const & clearValue
		, ImageLayout finalLayout )
	{
		return [clearValue, dstView, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, [[maybe_unused]] uint32_t index )
			{
				recctx::clearAttachment( recContext, commandBuffer, dstView, clearValue, finalLayout );
			};
	}

	//************************************************************************************************
}
