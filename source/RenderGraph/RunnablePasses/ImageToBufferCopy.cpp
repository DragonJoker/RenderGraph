/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageToBufferCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace
	{
		VkImageSubresourceLayers convert( VkImageSubresourceRange const & range )
		{
			return VkImageSubresourceLayers{ range.aspectMask
				, range.baseMipLevel
				, range.baseArrayLayer
				, range.layerCount };
		}
	}

	ImageToBufferCopy::ImageToBufferCopy( FramePass const & pass
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
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, passIndex
				, isEnabled }
			, std::move( ruConfig ) }
		, m_copyOffset{ std::move( copyOffset ) }
		, m_copySize{ std::move( copySize ) }
	{
	}

	void ImageToBufferCopy::doInitialise()
	{
	}

	void ImageToBufferCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.back().view() };
		auto dstBuffer{ m_pass.buffers.front().buffer.buffer.buffer };
		auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
		// Copy source to target.
		auto range = convert( srcAttach.data->info.subresourceRange );
		VkBufferImageCopy copyRegion{ 0ull
			, 0u
			, 0u
			, range
			, m_copyOffset
			, m_copySize };
		m_context.vkCmdCopyImageToBuffer( commandBuffer
			, srcImage
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, dstBuffer
			, 1u
			, &copyRegion );
	}

	VkPipelineStageFlags ImageToBufferCopy::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
}