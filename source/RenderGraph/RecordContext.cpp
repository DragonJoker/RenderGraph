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
			, ImageType type
			, PixelFormat format
			, ImageSubresourceRange const & subresourceRange )
		{
			ImageSubresourceRange result = subresourceRange;

			if ( type == ImageType::e3D )
			{
				result.baseArrayLayer = 0;
				result.layerCount = 1;
			}

			if ( !context.separateDepthStencilLayouts
				&& isDepthStencilFormat( format )
				&& ( checkFlag( result.aspectMask, ImageAspectFlags::eDepth )
					|| checkFlag( result.aspectMask, ImageAspectFlags::eStencil ) ) )
				result.aspectMask = ImageAspectFlags::eDepthStencil;

			return result;
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

	void RecordContext::registerImplicitTransition( RunnablePass const & pass
		, BufferViewId view
		, RecordContext::ImplicitAction action )
	{
		registerImplicitTransition( { &pass, view, std::move( action ) } );
	}

	void RecordContext::registerImplicitTransition( ImplicitImageTransition transition )
	{
		m_implicitImageTransitions.emplace_back( std::move( transition ) );
	}

	void RecordContext::registerImplicitTransition( ImplicitBufferTransition transition )
	{
		m_implicitBufferTransitions.emplace_back( std::move( transition ) );
	}

	void RecordContext::runImplicitTransition( VkCommandBuffer commandBuffer
		, uint32_t index
		, ImageViewId view )
	{
		auto it = std::find_if( m_implicitImageTransitions.begin()
			, m_implicitImageTransitions.end()
			, [&view]( ImplicitImageTransition const & lookup )
			{
				return lookup.view == view;
			} );

		if ( it != m_implicitImageTransitions.end() )
		{
			auto pass = it->pass;
			auto action = it->action;
			m_implicitImageTransitions.erase( it );

			if ( !pass->isEnabled() )
			{
				action( *this, commandBuffer, index );
			}
		}
	}

	void RecordContext::runImplicitTransition( VkCommandBuffer commandBuffer
		, uint32_t index
		, BufferViewId view )
	{
		auto it = std::find_if( m_implicitBufferTransitions.begin()
			, m_implicitBufferTransitions.end()
			, [&view]( ImplicitBufferTransition const & lookup )
			{
				return lookup.view == view;
			} );

		if ( it != m_implicitBufferTransitions.end() )
		{
			auto pass = it->pass;
			auto action = it->action;
			m_implicitBufferTransitions.erase( it );

			if ( !pass->isEnabled() )
			{
				action( *this, commandBuffer, index );
			}
		}
	}

	void RecordContext::setAccessState( BufferViewId buffer
		, AccessState const & accessState )
	{
		return setAccessState( buffer.data->buffer, getSubresourceRange( buffer ), accessState );
	}

	AccessState RecordContext::getAccessState( BufferViewId buffer )const
	{
		return getAccessState( buffer.data->buffer, getSubresourceRange( buffer ) );
	}

	void RecordContext::setAccessState( BufferId buffer
		, [[maybe_unused]] BufferSubresourceRange const & subresourceRange
		, AccessState const & accessState )
	{
		m_buffers.insert_or_assign( buffer.id, accessState );
	}

	AccessState const & RecordContext::getAccessState( BufferId buffer
		, [[maybe_unused]] BufferSubresourceRange const & subresourceRange )const
	{
		if ( auto bufferIt = m_buffers.find( buffer.id ); bufferIt != m_buffers.end() )
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
			, getSubresourceRange( view )
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
		auto range = recctx::adaptRange( m_resources->getContext()
				, image.data->info.imageType
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
			, getSubresourceRange( view )
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
		, BufferViewId view
		, AccessState const & initialState
		, AccessState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, view.data->buffer
			, getSubresourceRange( view )
			, initialState
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, BufferId buffer
		, BufferSubresourceRange const & subresourceRange
		, AccessState const & initialState
		, AccessState const & wantedState
		, bool force )
	{
		auto from = getAccessState( buffer
			, subresourceRange );

		if ( checkFlag( from.pipelineStage, PipelineStageFlags::eBottomOfPipe ) )
		{
			from = initialState;
		}

		if ( force
			|| ( from.access != wantedState.access
				|| from.pipelineStage != wantedState.pipelineStage ) )
		{
			auto & resources = getResources();
			VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
				, nullptr
				, getAccessFlags( from.access )
				, getAccessFlags( wantedState.access )
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, resources.createBuffer( buffer )
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
		, BufferId buffer
		, BufferSubresourceRange const & subresourceRange
		, AccessState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, buffer
			, subresourceRange
			, { AccessFlags::eNone, PipelineStageFlags::eBottomOfPipe }
			, wantedState
			, force );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, BufferViewId view
		, AccessState const & wantedState
		, bool force )
	{
		memoryBarrier( commandBuffer
			, view.data->buffer
			, getSubresourceRange( view )
			, wantedState
			, force );
	}

	GraphContext & RecordContext::getContext()const
	{
		return getResources().getContext();
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

	void RecordContext::copyImage( VkCommandBuffer commandBuffer
		, uint32_t index
		, ImageViewId srcView
		, ImageViewId dstView
		, Extent2D const & extent
		, ImageLayout finalLayout )
	{
		runImplicitTransition( commandBuffer, index, srcView );
		auto & srcSubresource = getSubresourceRange( srcView );
		auto & dstSubresource = getSubresourceRange( dstView );
		VkImageCopy region{ getSubresourceLayer( srcSubresource ), VkOffset3D{ 0u, 0u, 0u }
			, getSubresourceLayer( dstSubresource ), VkOffset3D{ 0u, 0u, 0u }
		, { extent.width, extent.height, 1u } };
		memoryBarrier( commandBuffer, srcView, makeLayoutState( ImageLayout::eTransferSrc ) );
		memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
		auto & resources = getResources();
		resources->vkCmdCopyImage( commandBuffer
			, resources.createImage( srcView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u, &region );

		if ( finalLayout != ImageLayout::eUndefined )
			memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
	}

	void RecordContext::blitImage( VkCommandBuffer commandBuffer
		, uint32_t index
		, ImageViewId srcView
		, ImageViewId dstView
		, Rect2D const & srcRect
		, Rect2D const & dstRect
		, FilterMode filter
		, ImageLayout finalLayout )
	{
		runImplicitTransition( commandBuffer, index, srcView );
		auto & srcSubresource = getSubresourceRange( srcView );
		auto & dstSubresource = getSubresourceRange( dstView );
		memoryBarrier( commandBuffer, srcView, makeLayoutState( ImageLayout::eTransferSrc ) );
		memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
		auto & resources = getResources();
		VkImageBlit region{ getSubresourceLayer( srcSubresource ), { VkOffset3D{ srcRect.offset.x, srcRect.offset.y, 0u }, VkOffset3D{ int32_t( srcRect.extent.width ), int32_t( srcRect.extent.height ), 1 } }
			, getSubresourceLayer( dstSubresource ), { VkOffset3D{ dstRect.offset.x, dstRect.offset.y, 0u }, VkOffset3D{ int32_t( dstRect.extent.width ), int32_t( dstRect.extent.height ), 1 } } };
		resources->vkCmdBlitImage( commandBuffer
			, resources.createImage( srcView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u, &region, convert( filter ) );

		if ( finalLayout != ImageLayout::eUndefined )
			memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
	}

	void RecordContext::clearAttachment( VkCommandBuffer commandBuffer
		, ImageViewId dstView
		, ClearColorValue const & clearValue
		, ImageLayout finalLayout )
	{
		auto & resources = getResources();
		memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
		auto subresourceRange = convert( getSubresourceRange( dstView ) );
		assert( isColourFormat( getFormat( dstView ) ) );
		auto vkClearValue = convert( clearValue );
		resources->vkCmdClearColorImage( commandBuffer
			, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, &vkClearValue, 1u, &subresourceRange );

		if ( finalLayout != ImageLayout::eUndefined )
			memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
	}

	void RecordContext::clearAttachment( VkCommandBuffer commandBuffer
		, ImageViewId dstView
		, ClearDepthStencilValue const & clearValue
		, ImageLayout finalLayout )
	{
		auto & resources = getResources();
		memoryBarrier( commandBuffer, dstView, makeLayoutState( ImageLayout::eTransferDst ) );
		auto subresourceRange = convert( getSubresourceRange( dstView ) );
		assert( isDepthOrStencilFormat( getFormat( dstView ) ) );
		auto vkClearValue = convert( clearValue );
		resources->vkCmdClearDepthStencilImage( commandBuffer
			, resources.createImage( dstView.data->image ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, &vkClearValue, 1u, &subresourceRange );

		if ( finalLayout != ImageLayout::eUndefined )
			memoryBarrier( commandBuffer, dstView, makeLayoutState( finalLayout ) );
	}

	void RecordContext::copyBuffer( VkCommandBuffer commandBuffer
		, uint32_t index
		, BufferViewId srcView
		, BufferViewId dstView
		, DeviceSize srcOffset, DeviceSize dstOffset
		, DeviceSize size
		, AccessState const & finalState )
	{
		runImplicitTransition( commandBuffer, index, srcView );
		memoryBarrier( commandBuffer, srcView, { AccessFlags::eTransferRead, PipelineStageFlags::eTransfer } );
		memoryBarrier( commandBuffer, dstView, { AccessFlags::eTransferWrite, PipelineStageFlags::eTransfer } );
		auto & resources = getResources();
		VkBufferCopy region{ srcOffset, dstOffset, size };
		resources->vkCmdCopyBuffer( commandBuffer
			, resources.createBuffer( srcView.data->buffer )
			, resources.createBuffer( dstView.data->buffer )
			, 1u, &region );

		if ( finalState != AccessState{} )
			memoryBarrier( commandBuffer, dstView, finalState );
	}

	void RecordContext::clearBuffer( VkCommandBuffer commandBuffer
		, BufferViewId dstView
		, uint32_t clearValue
		, AccessState const & finalState )
	{
		auto & resources = getResources();
		memoryBarrier( commandBuffer, dstView, { AccessFlags::eTransferWrite, PipelineStageFlags::eTransfer } );
		auto subresourceRange = getSubresourceRange( dstView );
		resources->vkCmdFillBuffer( commandBuffer
			, resources.createBuffer( dstView.data->buffer )
			, subresourceRange.offset, subresourceRange.size
			, clearValue );

		if ( finalState != AccessState{} )
			memoryBarrier( commandBuffer, dstView, finalState );
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
			recContext.copyImage( commandBuffer, index, srcView, dstView, extent, finalLayout );
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
			recContext.blitImage( commandBuffer, index, srcView, dstView, srcRect, dstRect, filter, finalLayout );
		};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( Attachment const & attach
		, ImageLayout finalLayout )
	{
		return [&attach, finalLayout]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			auto dstView = attach.view( index );
			if ( isColourFormat( getFormat( dstView ) ) )
				recContext.clearAttachment( commandBuffer, dstView, getClearColorValue( attach.getClearValue() ), finalLayout );
			else
				recContext.clearAttachment( commandBuffer, dstView, getClearDepthStencilValue( attach.getClearValue() ), finalLayout );
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
				recContext.clearAttachment( commandBuffer, dstView, clearValue, finalLayout );
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
				recContext.clearAttachment( commandBuffer, dstView, clearValue, finalLayout );
			};
	}

	RecordContext::ImplicitAction RecordContext::clearBuffer( BufferViewId dstView
		, AccessState const & finalState )
	{
		return clearBuffer( dstView, 0u, finalState );
	}

	RecordContext::ImplicitAction RecordContext::clearBuffer( BufferViewId dstView
		, uint32_t clearValue
		, AccessState const & finalState )
	{
		return [clearValue, dstView, finalState]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, [[maybe_unused]] uint32_t index )
			{
				recContext.clearBuffer( commandBuffer, dstView, clearValue, finalState );
			};
	}

	RecordContext::ImplicitAction RecordContext::copyBuffer( BufferViewId srcView
		, BufferViewId dstView
		, DeviceSize srcOffset, DeviceSize dstOffset
		, DeviceSize size
		, AccessState const & finalState )
	{
		return [srcOffset, dstOffset, size, srcView, dstView, finalState]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
			{
				recContext.copyBuffer( commandBuffer, index, srcView, dstView, srcOffset, dstOffset, size, finalState );
			};
	}

	//************************************************************************************************
}
