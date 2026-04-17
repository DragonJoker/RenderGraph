/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/BufferToImageCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	BufferToImageCopy::BufferToImageCopy( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Offset3D const & copyOffset
		, Extent3D const & copySize
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( PipelineStageFlags::eTransfer ); } )
				, [this]( RecordContext const & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, std::move( passIndex )
				, std::move( isEnabled ) }
			, std::move( ruConfig ) }
		, m_copyOffset{ convert( copyOffset ) }
		, m_copySize{ convert( copySize ) }
	{
		if ( getPass().getInputs().size() != getPass().getOutputs().size() )
		{
			Logger::logError( "BufferToImageCopy - Inputs and outputs sizes are different." );
		}
	}

	void BufferToImageCopy::doRecordInto( RecordContext const & context
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
			auto srcAttach{ srcIt->second->buffer( index ) };
			auto dstAttach{ dstIt->second->view( index ) };
			auto dstImage{ &getGraph().createImage( dstAttach.data->image ) };
			auto srcBuffer{ &getGraph().createBuffer( srcAttach.data->buffer ) };
			if ( srcAttach.data->buffer.data->maxPages > 1 )
			{
				Logger::logWarning( "BufferToImageCopy - Source buffer [" + srcAttach.data->name + "] has more than one page, only the first one will be used." );
			}

			// Copy source to target.
			auto range = getSubresourceLayers( getSubresourceRange( dstAttach ) );
			VkBufferImageCopy copyRegion{ 0ULL
				, 0u
				, 0u
				, range
				, m_copyOffset
				, m_copySize };
			context->vkCmdCopyBufferToImage( commandBuffer
				, srcBuffer->getBuffer()
				, dstImage->getImage()
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &copyRegion );
			++srcIt;
			++dstIt;
		}
	}
}
