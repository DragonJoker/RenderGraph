/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/BufferCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	BufferCopy::BufferCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkDeviceSize copyOffset
		, VkDeviceSize copyRange
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { [this]( uint32_t ){ doInitialise(); }
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( VK_PIPELINE_STAGE_TRANSFER_BIT ); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, passIndex
				, isEnabled }
			, std::move( ruConfig ) }
		, m_copyOffset{ std::move( copyOffset ) }
		, m_copyRange{ std::move( copyRange ) }
	{
	}

	void BufferCopy::doInitialise()
	{
	}

	void BufferCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcBufferRange{ m_pass.buffers.front().buffer.range };
		auto dstBufferRange{ m_pass.buffers.back().buffer.range };
		auto srcBuffer{ m_pass.buffers.front().buffer.buffer.buffer };
		auto dstBuffer{ m_pass.buffers.back().buffer.buffer.buffer };
		// Copy source to target.
		VkBufferCopy copyRegion{ srcBufferRange.offset + m_copyOffset
			, dstBufferRange.offset + m_copyOffset
			, m_copyRange };
		context.memoryBarrier( commandBuffer
			, srcBuffer
			, srcBufferRange
			, VK_ACCESS_SHADER_WRITE_BIT
			, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			, crg::AccessState{ VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT } );
		context.memoryBarrier( commandBuffer
			, dstBuffer
			, dstBufferRange
			, crg::AccessState{ VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT } );
		m_context.vkCmdCopyBuffer( commandBuffer
			, srcBuffer
			, dstBuffer
			, 1u
			, &copyRegion );
		context.memoryBarrier( commandBuffer
			, dstBuffer
			, dstBufferRange
			, crg::AccessState{ VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
		context.memoryBarrier( commandBuffer
			, srcBuffer
			, srcBufferRange
			, crg::AccessState{ VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT } );
	}
}
