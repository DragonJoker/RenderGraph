/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/ImageCopy.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace
	{
		VkImageSubresourceLayers convert( VkImageSubresourceRange const & range )
		{
			return VkImageSubresourceLayers{ range.aspectMask
				, range.baseMipLevel
				, range.baseArrayLayer
				, range.layerCount };
		}
	}

	ImageCopy::ImageCopy( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, VkExtent3D copySize
		, uint32_t maxPassCount
		, uint32_t const * passIndex )
		: RunnablePass{ pass
			, context
			, graph
			, maxPassCount }
		, m_copySize{std::move( copySize ) }
		, m_passIndex{ passIndex }
	{
		assert( pass.images.size() == 2u );
	}

	ImageCopy::~ImageCopy()
	{
	}

	void ImageCopy::doInitialise()
	{
	}

	void ImageCopy::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.front().view() };
		auto dstAttach{ m_pass.images.back().view() };
		auto srcImage{ m_graph.getImage( srcAttach ) };
		auto dstImage{ m_graph.getImage( dstAttach ) };
		auto srcTransition = doGetTransition( index, srcAttach );
		auto dstTransition = doGetTransition( index, dstAttach );
		// Copy source to target.
		VkImageCopy copyRegion{ convert( srcAttach.data->info.subresourceRange )
			, {}
			, convert( dstAttach.data->info.subresourceRange )
			, {}
			, m_copySize };
		m_context.vkCmdCopyImage( commandBuffer
			, srcImage
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, dstImage
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u
			, &copyRegion );
	}

	VkPipelineStageFlags ImageCopy::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	uint32_t ImageCopy::doGetPassIndex()const
	{
		return m_passIndex
			? *m_passIndex
			: 0u;
	}
}
