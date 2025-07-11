/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageBlit.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	ImageBlit::ImageBlit( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Offset3D const & blitSrcOffset
		, Extent3D const & blitSrcSize
		, Offset3D const & blitDstOffset
		, Extent3D const & blitDstSize
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
		, m_srcOffset{ convert( blitSrcOffset ) }
		, m_srcSize{ convert( blitSrcSize ) }
		, m_dstOffset{ convert( blitDstOffset ) }
		, m_dstSize{ convert( blitDstSize ) }
		, m_filter{ filter }
	{
		assert( pass.images.size() == 2u );
	}

	void ImageBlit::doRecordInto( RecordContext const & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.front().view( index ) };
		auto dstAttach{ m_pass.images.back().view( index ) };
		auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		VkImageBlit blitRegion{ getSubresourceLayers( srcAttach.data->info.subresourceRange )
			, { m_srcOffset, VkOffset3D{ int32_t( m_srcSize.width ), int32_t( m_srcSize.height ), int32_t( m_srcSize.depth ) } }
			, getSubresourceLayers( dstAttach.data->info.subresourceRange )
			, { m_dstOffset, VkOffset3D{ int32_t( m_dstSize.width ), int32_t( m_dstSize.height ), int32_t( m_dstSize.depth ) } } };
		context->vkCmdBlitImage( commandBuffer
			, srcImage
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, dstImage
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u
			, &blitRegion
			, convert( m_filter ) );
	}
}
