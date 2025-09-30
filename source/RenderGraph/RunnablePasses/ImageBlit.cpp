/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageBlit.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	ImageBlit::ImageBlit( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Rect3D const & blitSrc
		, Rect3D const & blitDst
		, FilterMode filter
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
		, m_srcOffset{ convert( blitSrc.offset ) }
		, m_srcSize{ convert( blitSrc.extent ) }
		, m_dstOffset{ convert( blitDst.offset ) }
		, m_dstSize{ convert( blitDst.extent ) }
		, m_filter{ filter }
	{
		assert( getPass().getInputs().size() == getPass().getOutputs().size() );
	}

	void ImageBlit::doRecordInto( RecordContext const & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcIt = getPass().getInputs().begin();
		auto dstIt = getPass().getOutputs().begin();

		while ( srcIt != getPass().getInputs().end()
			&& dstIt != getPass().getOutputs().end() )
		{
			auto srcAttach{ srcIt->second->view( index ) };
			auto dstAttach{ dstIt->second->view( index ) };
			auto srcImage{ getGraph().createImage( srcAttach.data->image ) };
			auto dstImage{ getGraph().createImage( dstAttach.data->image ) };
			VkImageBlit blitRegion{ getSubresourceLayers( getSubresourceRange( srcAttach ) )
				, { m_srcOffset, VkOffset3D{ int32_t( m_srcSize.width ), int32_t( m_srcSize.height ), int32_t( m_srcSize.depth ) } }
				, getSubresourceLayers( getSubresourceRange( dstAttach ) )
				, { m_dstOffset, VkOffset3D{ int32_t( m_dstSize.width ), int32_t( m_dstSize.height ), int32_t( m_dstSize.depth ) } } };
			context->vkCmdBlitImage( commandBuffer
				, srcImage
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, dstImage
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				, 1u
				, &blitRegion
				, convert( m_filter ) );
			++srcIt;
			++dstIt;
		}
	}
}
