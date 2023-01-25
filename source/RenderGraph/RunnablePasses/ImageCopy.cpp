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
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_TRANSFER_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, passIndex
				, isEnabled }
			, std::move( ruConfig ) }
		, m_copySize{std::move( copySize ) }
	{
		assert( pass.images.size() == 2u );
	}

	void ImageCopy::doInitialise()
	{
	}

	void ImageCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.front().view( index ) };
		auto dstAttach{ m_pass.images.back().view( index ) };
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
	}
}
