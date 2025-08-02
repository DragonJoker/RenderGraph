/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/BufferCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
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
		assert( m_pass.inputs.size() == m_pass.outputs.size() );
	}

	void BufferCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		auto srcIt = m_pass.inputs.begin();
		auto dstIt = m_pass.outputs.begin();

		while ( srcIt != m_pass.inputs.end()
			&& dstIt != m_pass.outputs.end() )
		{
			auto srcView{ srcIt->second->buffer( index ) };
			auto dstView{ dstIt->second->buffer( index ) };
			auto srcBufferRange{ getSubresourceRange( srcView ) };
			auto dstBufferRange{ getSubresourceRange( dstView ) };
			auto srcBuffer{ m_graph.createBuffer( srcView.data->buffer ) };
			auto dstBuffer{ m_graph.createBuffer( dstView.data->buffer ) };
			// Copy source to target.
			VkBufferCopy copyRegion{ srcBufferRange.offset + m_copyOffset
				, dstBufferRange.offset + m_copyOffset
				, m_copyRange };
			context.memoryBarrier( commandBuffer
				, srcView
				, { AccessFlags::eShaderWrite, PipelineStageFlags::eFragmentShader }
				, { AccessFlags::eTransferRead, PipelineStageFlags::eTransfer } );
			context.memoryBarrier( commandBuffer
				, dstView
				, { AccessFlags::eTransferWrite, PipelineStageFlags::eTransfer } );
			context->vkCmdCopyBuffer( commandBuffer
				, srcBuffer
				, dstBuffer
				, 1u
				, &copyRegion );
			context.memoryBarrier( commandBuffer
				, dstView
				, { AccessFlags::eShaderRead, PipelineStageFlags::eComputeShader } );
			context.memoryBarrier( commandBuffer
				, srcView
				, { AccessFlags::eShaderWrite, PipelineStageFlags::eFragmentShader } );
			++srcIt;
			++dstIt;
		}
	}
}
