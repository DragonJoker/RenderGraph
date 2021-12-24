/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RecordContext.hpp"
#include "RenderGraph/ResourceHandler.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_set>

#pragma GCC diagnostic ignored "-Wnull-dereference"

namespace crg
{
	//************************************************************************************************

	namespace
	{
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
	}

	//************************************************************************************************

	VkImageAspectFlags getAspectMask( VkFormat format )noexcept
	{
		return VkImageAspectFlags( isDepthStencilFormat( format )
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

	LayoutState addSubresourceRangeLayout( LayerLayoutStates & ranges
		, VkImageSubresourceRange const & range
		, LayoutState const & newLayout )
	{
		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto & layers = ranges.emplace( range.baseArrayLayer + layerIdx, MipLayoutStates{} ).first->second;

			for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
			{
				auto & level = layers.emplace( range.baseMipLevel + levelIdx, LayoutState{} ).first->second;
				level.layout = newLayout.layout;
				level.access = newLayout.access;
				level.pipelineStage = newLayout.pipelineStage;
			}
		}

		return newLayout;
	}

	LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, VkImageSubresourceRange const & range )
	{
		std::map< VkImageLayout, LayoutState > states;

		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto layerIt = ranges.find( range.baseArrayLayer + layerIdx );
				
			if ( layerIt != ranges.end() )
			{
				auto & layers = layerIt->second;

				for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
				{
					auto it = layers.find( range.baseMipLevel + levelIdx );

					if ( it != layers.end() )
					{
						auto state = it->second;
						auto ires = states.emplace( state.layout, state );

						if ( !ires.second )
						{
							ires.first->second.access |= state.access;
						}
					}
				}
			}
		}

		if ( states.empty() )
		{
			return { VK_IMAGE_LAYOUT_UNDEFINED
				, getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
				, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };
		}

		if ( states.size() == 1u )
		{
			return states.begin()->second;
		}

		return states.begin()->second;
	}

	//************************************************************************************************

	void RecordData::setLayoutState( crg::ImageViewId view
		, LayoutState layoutState )
	{
		setLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, layoutState );
	}

	LayoutState RecordData::getLayoutState( ImageViewId view )const
	{
		return getLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange );
	}

	void RecordData::setLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState layoutState )
	{
		auto range = getVirtualRange( image
			, viewType
			, subresourceRange );
		auto ires = m_images.emplace( image.id, LayerLayoutStates{} );
		addSubresourceRangeLayout( ires.first->second
			, range
			, layoutState );
	}

	LayoutState RecordData::getLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange )const
	{
		auto imageIt = m_images.find( image.id );

		if ( imageIt != m_images.end() )
		{
			auto range = getVirtualRange( image
				, viewType
				, subresourceRange );
			return getSubresourceRangeLayout( imageIt->second
				, range );
		}

		return { VK_IMAGE_LAYOUT_UNDEFINED, 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	}

	void RecordData::setAccessState( VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, AccessState layoutState )
	{
		auto ires = m_buffers.emplace( buffer, AccessState{} );
		ires.first->second = layoutState;
	}

	AccessState RecordData::getAccessState( VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange )const
	{
		auto bufferIt = m_buffers.find( buffer );

		if ( bufferIt != m_buffers.end() )
		{
			return bufferIt->second;
		}

		return { 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	}

	//************************************************************************************************

	RecordContext::RecordContext( ResourceHandler & handler
		, GraphContext & context )
		: m_handler{ &handler }
		, m_context{ &context }
	{
	}

	void RecordContext::setLayoutState( crg::ImageViewId view
		, LayoutState layoutState )
	{
		setLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, layoutState );
	}

	LayoutState RecordContext::getLayoutState( ImageViewId view )const
	{
		return getLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange );
	}

	void RecordContext::setLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState layoutState )
	{
		m_data.setLayoutState( image
			, viewType
			, subresourceRange
			, layoutState );
	}

	LayoutState RecordContext::getLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange )const
	{
		return m_data.getLayoutState( image
			, viewType
			, subresourceRange );
	}

	void RecordContext::registerImplicitTransition( RunnablePass const & pass
			, crg::ImageViewId view
			, RecordContext::ImplicitAction action )
	{
		registerImplicitTransition( { &pass, view, action } );
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
		, BufferSubresourceRange const & subresourceRange
		, AccessState layoutState )
	{
		m_data.setAccessState( buffer
			, subresourceRange
			, layoutState );
	}

	AccessState RecordContext::getAccessState( VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange )const
	{
		return m_data.getAccessState( buffer
			, subresourceRange );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageViewId const & view
		, VkImageLayout initialLayout
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, initialLayout
			, wantedState );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageSubresourceRange const & subresourceRange
		, VkImageLayout initialLayout
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, image
			, VkImageViewType( image.data->info.imageType )
			, subresourceRange
			, initialLayout
			, wantedState );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, VkImageLayout initialLayout
		, LayoutState const & wantedState )
	{
		if ( !m_context->device )
		{
			return;
		}

		auto range = adaptRange( *m_context
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

		if ( from.layout != wantedState.layout
			&& wantedState.layout != VK_IMAGE_LAYOUT_UNDEFINED )
		{
			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
				, nullptr
				, from.access
				, wantedState.access
				, from.layout
				, wantedState.layout
				, VK_QUEUE_FAMILY_IGNORED
				, VK_QUEUE_FAMILY_IGNORED
				, m_handler->createImage( *m_context, image )
				, range };
			m_context->vkCmdPipelineBarrier( commandBuffer
				, from.pipelineStage
				, wantedState.pipelineStage
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
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, VK_IMAGE_LAYOUT_UNDEFINED
			, wantedState );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, image
			, VkImageViewType( image.data->info.imageType )
			, subresourceRange
			, VK_IMAGE_LAYOUT_UNDEFINED
			, wantedState );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, image
			, viewType
			, subresourceRange
			, VK_IMAGE_LAYOUT_UNDEFINED
			, wantedState );
	}

	void RecordContext::memoryBarrier( VkCommandBuffer commandBuffer
		, VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, VkAccessFlags initialMask
		, VkPipelineStageFlags initialStage
		, AccessState const & wantedState )
	{
		if ( !m_context->device )
		{
			return;
		}

		auto from = getAccessState( buffer
			, subresourceRange );

		if ( from.pipelineStage == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT )
		{
			from = { initialMask, initialStage };
		}

		if ( from.access != wantedState.access
			|| from.pipelineStage != wantedState.pipelineStage )
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
			m_context->vkCmdPipelineBarrier( commandBuffer
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
		, AccessState const & wantedState )
	{
		memoryBarrier( commandBuffer
			, buffer
			, subresourceRange
			, 0u
			, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
			, wantedState );
	}

	LayoutState makeLayoutState( VkImageLayout layout )
	{
		return { layout
			, crg::getAccessMask( layout )
			, crg::getStageMask( layout ) };
	}

	RecordContext::ImplicitAction RecordContext::copyImage( ImageViewId srcView
		, ImageViewId dstView
		, VkExtent2D extent )
	{
		return [srcView, dstView, extent]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
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
			recContext.m_context->vkCmdCopyImage( commandBuffer
				, recContext.m_handler->createImage( *recContext.m_context, srcView.data->image )
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, recContext.m_handler->createImage( *recContext.m_context, dstView.data->image )
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &region );
		};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( Attachment attach )
	{
		return [attach]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			auto dstView = attach.view( index );
			recContext.memoryBarrier( commandBuffer
				, dstView.data->image
				, dstView.data->info.viewType
				, dstView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );

			if ( isColourFormat( getFormat( dstView ) ) )
			{
				auto colour = attach.image.clearValue.color;
				recContext.m_context->vkCmdClearColorImage( commandBuffer
					, recContext.m_handler->createImage( *recContext.m_context, dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &colour
					, 1u
					, &dstView.data->info.subresourceRange );
			}
			else
			{
				auto depthStencil = attach.image.clearValue.depthStencil;
				recContext.m_context->vkCmdClearDepthStencilImage( commandBuffer
					, recContext.m_handler->createImage( *recContext.m_context, dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &depthStencil
					, 1u
					, &dstView.data->info.subresourceRange );
			}
		};
	}

	RecordContext::ImplicitAction RecordContext::clearAttachment( ImageViewId dstView
		, VkClearValue const & clearValue )
	{
		return [clearValue, dstView]( RecordContext & recContext
			, VkCommandBuffer commandBuffer
			, uint32_t index )
		{
			recContext.memoryBarrier( commandBuffer
				, dstView.data->image
				, dstView.data->info.viewType
				, dstView.data->info.subresourceRange
				, makeLayoutState( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );

			if ( isColourFormat( getFormat( dstView ) ) )
			{
				auto colour = clearValue.color;
				recContext.m_context->vkCmdClearColorImage( commandBuffer
					, recContext.m_handler->createImage( *recContext.m_context, dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &colour
					, 1u
					, &dstView.data->info.subresourceRange );
			}
			else
			{
				auto depthStencil = clearValue.depthStencil;
				recContext.m_context->vkCmdClearDepthStencilImage( commandBuffer
					, recContext.m_handler->createImage( *recContext.m_context, dstView.data->image )
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, &depthStencil
					, 1u
					, &dstView.data->info.subresourceRange );
			}
		};
	}

	//************************************************************************************************
}
