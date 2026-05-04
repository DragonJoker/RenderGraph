/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageToBufferCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	ImageToBufferCopy::ImageToBufferCopy( FramePass const & pass
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
			, RunnablePass::Callbacks{}
				.onGetPipelineState( [](){ return crg::getPipelineState( PipelineStageFlags::eTransfer ); } )
				.onRecord( [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); } )
				.onGetPassIndex( std::move( passIndex ) )
				.onIsEnabled( std::move( isEnabled ) )
			, std::move( ruConfig ) }
		, m_copyOffset{ convert( copyOffset ) }
		, m_copySize{ convert( copySize ) }
	{
		if ( getPass().getInputs().size() != getPass().getOutputs().size() )
		{
			Logger::logError( "BufferCopy - Inputs and outputs sizes are different." );
		}
	}

	void ImageToBufferCopy::doRecordInto( RecordContext const & context
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
			auto srcAttach{ srcIt->second->view( index ) };
			auto dstAttach{ dstIt->second->buffer( index ) };
			auto srcImage{ &getGraph().createImage( srcAttach.data->image ) };
			auto dstBuffer{ &getGraph().createBuffer( dstAttach.data->buffer ) };

			if ( dstAttach.data->buffer.data->maxPages > 1 )
			{
				Logger::logWarning( "ImageToBufferCopy - Destination buffer [" + dstAttach.data->name + "] has more than one page, only the first one will be used." );
			}

			// Copy source to target.
			auto range = getSubresourceLayers( getSubresourceRange( srcAttach ) );
			VkBufferImageCopy copyRegion{ 0ULL
				, 0u
				, 0u
				, range
				, m_copyOffset
				, m_copySize };
			context->vkCmdCopyImageToBuffer( commandBuffer
				, srcImage->getImage()
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, dstBuffer->getBuffer()
				, 1u
				, &copyRegion );
			++srcIt;
			++dstIt;
		}
	}
}
