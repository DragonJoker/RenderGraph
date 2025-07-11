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
		, DeviceSize copyOffset
		, DeviceSize copyRange
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
		, m_copyOffset{ copyOffset }
		, m_copyRange{ copyRange }
	{
	}

	void BufferCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		auto srcBufferRange{ m_pass.buffers.front().getBufferRange() };
		auto dstBufferRange{ m_pass.buffers.back().getBufferRange() };
		auto srcBuffer{ m_pass.buffers.front().buffer( index ) };
		auto dstBuffer{ m_pass.buffers.back().buffer( index ) };
		// Copy source to target.
		VkBufferCopy copyRegion{ srcBufferRange.offset + m_copyOffset
			, dstBufferRange.offset + m_copyOffset
			, m_copyRange };
		context.memoryBarrier( commandBuffer
			, srcBuffer
			, srcBufferRange
			, { AccessFlags::eShaderWrite, PipelineStageFlags::eFragmentShader }
			, { AccessFlags::eTransferRead, PipelineStageFlags::eTransfer } );
		context.memoryBarrier( commandBuffer
			, dstBuffer
			, dstBufferRange
			, { AccessFlags::eTransferWrite, PipelineStageFlags::eTransfer } );
		context->vkCmdCopyBuffer( commandBuffer
			, srcBuffer
			, dstBuffer
			, 1u
			, &copyRegion );
		context.memoryBarrier( commandBuffer
			, dstBuffer
			, dstBufferRange
			, { AccessFlags::eShaderRead, PipelineStageFlags::eComputeShader } );
		context.memoryBarrier( commandBuffer
			, srcBuffer
			, srcBufferRange
			, { AccessFlags::eShaderWrite, PipelineStageFlags::eFragmentShader } );
	}
}
