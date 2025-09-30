/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageToBufferCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
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
			, { defaultV< InitialiseCallback >
				, GetPipelineStateCallback( [](){ return crg::getPipelineState( PipelineStageFlags::eTransfer ); } )
				, [this]( RecordContext const & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, std::move( passIndex )
				, std::move( isEnabled ) }
			, std::move( ruConfig ) }
		, m_copyOffset{ convert( copyOffset ) }
		, m_copySize{ convert( copySize ) }
	{
	}

	void ImageToBufferCopy::doRecordInto( RecordContext const & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		auto srcIt = getPass().getInputs().begin();
		auto dstIt = getPass().getOutputs().begin();

		while ( srcIt != getPass().getInputs().end()
			&& dstIt != getPass().getOutputs().end() )
		{
			auto srcAttach{ srcIt->second->view( index ) };
			auto dstAttach{ dstIt->second->buffer( index ) };
			auto srcImage{ getGraph().createImage( srcAttach.data->image ) };
			auto dstBuffer{ getGraph().createBuffer( dstAttach.data->buffer ) };
			// Copy source to target.
			auto range = getSubresourceLayers( getSubresourceRange( srcAttach ) );
			VkBufferImageCopy copyRegion{ 0ULL
				, 0u
				, 0u
				, range
				, m_copyOffset
				, m_copySize };
			context->vkCmdCopyImageToBuffer( commandBuffer
				, srcImage
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, dstBuffer
				, 1u
				, &copyRegion );
			++srcIt;
			++dstIt;
		}
	}
}
