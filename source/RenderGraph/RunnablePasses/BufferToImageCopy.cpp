/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/BufferToImageCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace bufToImg
	{
		static VkImageSubresourceLayers convert( VkImageSubresourceRange const & range )
		{
			return VkImageSubresourceLayers{ range.aspectMask
				, range.baseMipLevel
				, range.baseArrayLayer
				, range.layerCount };
		}
	}

	BufferToImageCopy::BufferToImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkOffset3D copyOffset
		, VkExtent3D copySize
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_TRANSFER_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, std::move( passIndex )
				, std::move( isEnabled ) }
			, std::move( ruConfig ) }
		, m_copyOffset{ copyOffset }
		, m_copySize{ copySize }
	{
	}

	void BufferToImageCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		auto dstAttach{ m_pass.images.back().view( index ) };
		auto srcBuffer{ m_pass.buffers.front().buffer.buffer.buffer( index ) };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		// Copy source to target.
		auto range = bufToImg::convert( dstAttach.data->info.subresourceRange );
		VkBufferImageCopy copyRegion{ 0ull
			, 0u
			, 0u
			, range
			, m_copyOffset
			, m_copySize };
		context->vkCmdCopyBufferToImage( commandBuffer
			, srcBuffer
			, dstImage
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u
			, &copyRegion );
	}
}
