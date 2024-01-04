/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace imgCopy
	{
		static VkImageSubresourceLayers convert( VkImageSubresourceRange const & range )
		{
			return VkImageSubresourceLayers{ range.aspectMask
				, range.baseMipLevel
				, range.baseArrayLayer
				, range.layerCount };
		}
	}

	ImageCopy::ImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkExtent3D copySize
		, VkImageLayout finalOutputLayout
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_TRANSFER_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, passIndex
				, isEnabled }
			, std::move( ruConfig ) }
		, m_copySize{std::move( copySize ) }
		, m_finalOutputLayout{ finalOutputLayout }
	{
		assert( ( pass.images.size() % 2u ) == 0u );
	}

	ImageCopy::ImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkExtent3D copySize
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: ImageCopy{ pass
			, context
			, graph
			, copySize
			, VK_IMAGE_LAYOUT_UNDEFINED
			, std::move( ruConfig )
			, passIndex
			, isEnabled }
	{
		assert( ( pass.images.size() % 2u ) == 0u );
	}

	void ImageCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcIt = m_pass.images.begin();
		auto dstIt = std::next( srcIt );

		while ( srcIt != m_pass.images.end() )
		{
			auto srcAttach{ srcIt->view( index ) };
			auto dstAttach{ dstIt->view( index ) };
			auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
			auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
			// Copy source to target.
			VkImageCopy copyRegion{ imgCopy::convert( srcAttach.data->info.subresourceRange )
				, {}
				, imgCopy::convert( dstAttach.data->info.subresourceRange )
				, {}
				, m_copySize };
			m_context.vkCmdCopyImage( commandBuffer
				, srcImage
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, dstImage
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &copyRegion );

			if ( m_finalOutputLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				context.memoryBarrier( commandBuffer
					, dstAttach
					, crg::makeLayoutState( m_finalOutputLayout ) );
			}

			srcIt = std::next( dstIt );

			if ( srcIt != m_pass.images.end() )
			{
				dstIt = std::next( srcIt );
			}
		}
	}
}
