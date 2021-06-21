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
		, GraphContext & context
		, RunnableGraph & graph
		, VkExtent3D copySize
		, uint32_t maxPassCount
		, bool optional
		, uint32_t const * passIndex
		, bool const * enabled )
		: RunnablePass{ pass
			, context
			, graph
			, maxPassCount
			, optional }
		, m_copySize{std::move( copySize ) }
		, m_passIndex{ passIndex }
		, m_enabled{ enabled }
	{
		assert( pass.images.size() == 2u );
	}

	ImageCopy::~ImageCopy()
	{
	}

	void ImageCopy::doInitialise( uint32_t index )
	{
	}

	void ImageCopy::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto srcAttach{ m_pass.images.front().view() };
		auto dstAttach{ m_pass.images.back().view() };
		auto srcImage{ m_graph.createImage( srcAttach.data->image ) };
		auto dstImage{ m_graph.createImage( dstAttach.data->image ) };
		auto srcTransition = getTransition( index, srcAttach );
		auto dstTransition = getTransition( index, dstAttach );
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

	bool ImageCopy::doIsEnabled()const
	{
		return m_enabled
			? *m_enabled
			: true;
	}
}
