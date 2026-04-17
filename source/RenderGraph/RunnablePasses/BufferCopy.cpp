/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/BufferCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/Log.hpp"
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
		if ( getPass().getInputs().size() != getPass().getOutputs().size() )
		{
			Logger::logError( "BufferCopy - Inputs and outputs sizes are different." );
		}
	}

	void BufferCopy::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		if ( getPass().getInputs().size() != getPass().getOutputs().size() )
		{
			return;
		}

		auto srcIt = getPass().getInputs().begin();
		auto dstIt = getPass().getOutputs().begin();

		while ( srcIt != getPass().getInputs().end()
			&& dstIt != getPass().getOutputs().end() )
		{
			auto srcView{ srcIt->second->buffer( index ) };
			auto dstView{ dstIt->second->buffer( index ) };
			auto srcBufferRange{ getSubresourceRange( srcView ) };
			auto dstBufferRange{ getSubresourceRange( dstView ) };
			auto [srcPageMin, srcPageCount] = getBufferPageRange( srcView.data->buffer, { srcBufferRange.offset + m_copyOffset, m_copyRange } );
			auto [dstPageMin, dstPageCount] = getBufferPageRange( dstView.data->buffer, { dstBufferRange.offset + m_copyOffset, m_copyRange } );

			if ( srcPageCount > 1 || dstPageCount > 1 )
			{
				Logger::logError( "BufferCopy - Source [" + srcView.data->name + "] or destination [" + dstView.data->name + "] buffer views have more than one page, this is currently unsupported." );
			}
			else
			{
				auto srcPageSize = srcView.data->buffer.data->info.size;
				auto dstPageSize = dstView.data->buffer.data->info.size;
				auto srcBuffer{ &getGraph().createBuffer( srcView.data->buffer ) };
				auto dstBuffer{ &getGraph().createBuffer( dstView.data->buffer ) };
				// Copy source to target.
				VkBufferCopy copyRegion{ srcBufferRange.offset + m_copyOffset - srcPageMin * srcPageSize
					, dstBufferRange.offset + m_copyOffset - dstPageMin * dstPageSize
					, m_copyRange };
				context.memoryBarrier( commandBuffer
					, srcView
					, { AccessFlags::eShaderWrite, PipelineStageFlags::eFragmentShader }
				, { AccessFlags::eTransferRead, PipelineStageFlags::eTransfer } );
				context.memoryBarrier( commandBuffer
					, dstView
					, { AccessFlags::eTransferWrite, PipelineStageFlags::eTransfer } );
				context->vkCmdCopyBuffer( commandBuffer
					, srcBuffer->getBuffer( srcPageMin )
					, dstBuffer->getBuffer( dstPageMin )
					, 1u
					, &copyRegion );
			}
			++srcIt;
			++dstIt;
		}
	}
}
