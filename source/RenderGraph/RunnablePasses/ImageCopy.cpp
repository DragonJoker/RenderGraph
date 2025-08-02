/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	ImageCopy::ImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Extent3D const & copySize
		, ImageLayout finalOutputLayout
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( PipelineStageFlags::eTransfer ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, std::move( passIndex )
				, std::move( isEnabled ) }
			, std::move( ruConfig ) }
		, m_copySize{ convert( copySize ) }
		, m_finalOutputLayout{ finalOutputLayout }
	{
		assert( m_pass.inputs.size() == m_pass.outputs.size() || m_pass.inputs.size() == 1u || m_pass.outputs.size() == 1u );
	}

	ImageCopy::ImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Extent3D const & copySize
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: ImageCopy{ pass
			, context
			, graph
			, copySize
			, ImageLayout::eUndefined
			, std::move( ruConfig )
			, std::move( passIndex )
			, std::move( isEnabled ) }
	{
		assert( m_pass.inputs.size() == m_pass.outputs.size() || m_pass.inputs.size() == 1u || m_pass.outputs.size() == 1u );
	}

	void ImageCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_pass.inputs.size() == m_pass.outputs.size() )
		{
			doRecordMultiToMulti( context, commandBuffer, index );
		}
		else if ( m_pass.outputs.size() == 1u )
		{
			doRecordMultiToSingle( context, commandBuffer, index );
		}
		else if ( m_pass.inputs.size() == 1u )
		{
			doRecordSingleToMulti( context, commandBuffer, index );
		}
	}

	void ImageCopy::doRecordMultiToMulti( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcIt = m_pass.inputs.begin();
		auto dstIt = m_pass.outputs.begin();

		while ( srcIt != m_pass.inputs.end()
			&& dstIt != m_pass.outputs.end() )
		{
			auto srcAttach{ srcIt->second->view( index ) };
			auto dstAttach{ dstIt->second->view( index ) };
			auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
			auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
			// Copy source to target.
			VkImageCopy copyRegion{ getSubresourceLayers( getSubresourceRange( srcAttach ) )
				, {}
				, getSubresourceLayers( getSubresourceRange( dstAttach ) )
				, {}
			, m_copySize };
			context->vkCmdCopyImage( commandBuffer
				, srcImage
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, dstImage
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &copyRegion );

			if ( m_finalOutputLayout != ImageLayout::eUndefined )
			{
				context.memoryBarrier( commandBuffer
					, dstAttach
					, crg::makeLayoutState( m_finalOutputLayout ) );
			}

			++srcIt;
			++dstIt;
		}
	}

	void ImageCopy::doRecordMultiToSingle( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		std::vector< VkImageCopy > copyRegions;
		auto dstIt = m_pass.outputs.begin();
		auto dstAttach{ dstIt->second->view( index ) };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		auto dstSubresourceRange = getSubresourceLayers( getSubresourceRange( dstAttach ) );
		auto prvSrcImage{ m_graph.createImage( m_pass.inputs.begin()->second->view( index ).data->image ) };

		for ( auto const & [_, attach] : m_pass.inputs )
		{
			auto srcAttach{ attach->view( index ) };

			if ( auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
				srcImage != prvSrcImage )
			{
				context->vkCmdCopyImage( commandBuffer
					, prvSrcImage
					, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, dstImage
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, uint32_t( copyRegions.size() )
					, copyRegions.data() );
				copyRegions.clear();
				prvSrcImage = srcImage;
			}

			copyRegions.push_back( { getSubresourceLayers( getSubresourceRange( srcAttach ) )
				, {}
				, dstSubresourceRange
				, {}
			, m_copySize } );
		}

		if ( !copyRegions.empty() )
		{
			context->vkCmdCopyImage( commandBuffer
				, prvSrcImage
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, dstImage
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, uint32_t( copyRegions.size() )
				, copyRegions.data() );
		}

		if ( m_finalOutputLayout != ImageLayout::eUndefined )
		{
			context.memoryBarrier( commandBuffer
				, dstAttach
				, crg::makeLayoutState( m_finalOutputLayout ) );
		}
	}

	void ImageCopy::doRecordSingleToMulti( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		std::vector< VkImageCopy > copyRegions;
		auto srcIt = m_pass.inputs.begin();
		auto srcAttach{ srcIt->second->view( index ) };
		auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
		auto srcSubresourceRange = getSubresourceLayers( getSubresourceRange( srcAttach ) );
		auto prvDstImage{ m_graph.createImage( m_pass.outputs.begin()->second->view( index ).data->image ) };

		for ( auto const & [_, attach] : m_pass.outputs )
		{
			auto dstAttach{ attach->view( index ) };

			if ( auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
				dstImage != prvDstImage )
			{
				context->vkCmdCopyImage( commandBuffer
					, srcImage
					, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, prvDstImage
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, uint32_t( copyRegions.size() )
					, copyRegions.data() );
				copyRegions.clear();
				prvDstImage = dstImage;
			}

			copyRegions.push_back( { srcSubresourceRange
				, {}
				, getSubresourceLayers( getSubresourceRange( dstAttach ) )
				, {}
			, m_copySize } );
		}

		if ( !copyRegions.empty() )
		{
			context->vkCmdCopyImage( commandBuffer
				, srcImage
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, prvDstImage
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, uint32_t( copyRegions.size() )
				, copyRegions.data() );
		}

		if ( m_finalOutputLayout != ImageLayout::eUndefined )
		{
			for ( auto const & [_, attach] : m_pass.outputs )
			{
				auto dstAttach{ attach->view( index ) };
				context.memoryBarrier( commandBuffer
					, dstAttach
					, crg::makeLayoutState( m_finalOutputLayout ) );
			}
		}
	}
}
