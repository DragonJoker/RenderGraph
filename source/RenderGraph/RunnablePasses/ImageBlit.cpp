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
	namespace imgBlit
	{
		static VkImageSubresourceLayers convert( VkImageSubresourceRange const & range )
		{
			return VkImageSubresourceLayers{ range.aspectMask
				, range.baseMipLevel
				, range.baseArrayLayer
				, range.layerCount };
		}
	}

	ImageBlit::ImageBlit( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkOffset3D blitSrcOffset
		, VkExtent3D blitSrcSize
		, VkOffset3D blitDstOffset
		, VkExtent3D blitDstSize
		, FilterMode filter
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
		, m_srcOffset{ std::move( blitSrcOffset ) }
		, m_srcSize{ std::move( blitSrcSize ) }
		, m_dstOffset{ std::move( blitDstOffset ) }
		, m_dstSize{ std::move( blitDstSize ) }
		, m_filter{ filter }
	{
		assert( pass.images.size() == 2u );
	}

	void ImageBlit::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.front().view( index ) };
		auto dstAttach{ m_pass.images.back().view( index ) };
		auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		VkImageBlit blitRegion{ imgBlit::convert( srcAttach.data->info.subresourceRange )
			, { m_srcOffset, VkOffset3D{ int32_t( m_srcSize.width ), int32_t( m_srcSize.height ), int32_t( m_srcSize.depth ) } }
			, imgBlit::convert( dstAttach.data->info.subresourceRange )
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
