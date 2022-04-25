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
		, VkFilter filter
		, ru::Config ruConfig
		, GetPassIndexCallback passIndex
		, IsEnabledCallback isEnabled )
		: RunnablePass{ pass
			, context
			, graph
			, { [this](){ doInitialise(); }
				, GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } )
				, [this]( RecordContext & recContext, VkCommandBuffer cb, uint32_t i ){ doRecordInto( recContext, cb, i ); }
				, passIndex
				, isEnabled }
			, std::move( ruConfig ) }
		, m_srcOffset{ std::move( blitSrcOffset ) }
		, m_srcSize{ std::move( blitSrcSize ) }
		, m_dstOffset{ std::move( blitDstOffset ) }
		, m_dstSize{ std::move( blitDstSize ) }
		, m_filter{ filter }
	{
		assert( pass.images.size() == 2u );
	}

	void ImageBlit::doInitialise()
	{
	}

	void ImageBlit::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.front().view() };
		auto dstAttach{ m_pass.images.back().view() };
		auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		VkImageBlit blitRegion{ imgBlit::convert( srcAttach.data->info.subresourceRange )
			, { m_srcOffset, VkOffset3D{ int32_t( m_srcSize.width ), int32_t( m_srcSize.height ), int32_t( m_srcSize.depth ) } }
			, imgBlit::convert( dstAttach.data->info.subresourceRange )
			, { m_dstOffset, VkOffset3D{ int32_t( m_dstSize.width ), int32_t( m_dstSize.height ), int32_t( m_dstSize.depth ) } } };
		m_context.vkCmdBlitImage( commandBuffer
			, srcImage
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, dstImage
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u
			, &blitRegion
			, m_filter );
	}

	VkPipelineStageFlags ImageBlit::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
}
