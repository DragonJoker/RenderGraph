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

	BufferToImageCopy::BufferToImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkOffset3D copyOffset
		, VkExtent3D copySize
		, ru::Config ruConfig
		, uint32_t const * passIndex
		, bool const * enabled )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, defaultV< RunnablePass::RecordCallback >
				, GetPassIndexCallback( [this](){ return doGetPassIndex(); } )
				, IsEnabledCallback( [this](){ return doIsEnabled(); } ) }
			, std::move( ruConfig ) }
		, m_copyOffset{ std::move( copyOffset ) }
		, m_copySize{ std::move( copySize ) }
		, m_passIndex{ passIndex }
		, m_enabled{ enabled }
	{
	}

	void BufferToImageCopy::doInitialise()
	{
	}

	void BufferToImageCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto dstAttach{ m_pass.images.back().view() };
		auto srcBuffer{ m_pass.buffers.front().buffer.buffer.buffer };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		// Copy source to target.
		auto range = convert( dstAttach.data->info.subresourceRange );
		VkBufferImageCopy copyRegion{ 0ull
			, 0u
			, 0u
			, range
			, m_copyOffset
			, m_copySize };
		m_context.vkCmdCopyBufferToImage( commandBuffer
			, srcBuffer
			, dstImage
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u
			, &copyRegion );
	}

	VkPipelineStageFlags BufferToImageCopy::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	uint32_t BufferToImageCopy::doGetPassIndex()const
	{
		return m_passIndex
			? *m_passIndex
			: 0u;
	}

	bool BufferToImageCopy::doIsEnabled()const
	{
		return m_enabled
			? *m_enabled
			: true;
	}
}
