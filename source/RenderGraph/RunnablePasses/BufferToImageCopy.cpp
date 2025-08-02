/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/BufferToImageCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
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
		assert( m_pass.inputs.size() == m_pass.outputs.size() );
	}

	void BufferToImageCopy::doRecordInto( RecordContext const & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		auto srcIt = m_pass.inputs.begin();
		auto dstIt = m_pass.outputs.begin();

		while ( srcIt != m_pass.inputs.end()
			&& dstIt != m_pass.outputs.end() )
		{
			auto srcAttach{ srcIt->second->buffer( index ) };
			auto dstAttach{ dstIt->second->view( index ) };
			auto srcBuffer{ m_graph.createBuffer( srcAttach.data->buffer ) };
			auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
			// Copy source to target.
			auto range = getSubresourceLayers( getSubresourceRange( dstAttach ) );
			VkBufferImageCopy copyRegion{ 0ULL
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
			++srcIt;
			++dstIt;
		}
	}
}
