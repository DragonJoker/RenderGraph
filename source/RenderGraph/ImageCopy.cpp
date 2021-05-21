/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/ImageCopy.hpp"

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
		, VkExtent3D copySize )
		: RunnablePass{ pass
			, context
			, graph }
		, m_srcAttach{ pass.transferInOuts.front() }
		, m_dstAttach{ pass.transferInOuts.back() }
		, m_copySize{std::move( copySize ) }
		, m_srcImage{ graph.getImage( m_srcAttach ) }
		, m_dstImage{ graph.getImage( m_dstAttach ) }
	{
	}

	ImageCopy::~ImageCopy()
	{
	}

	void ImageCopy::doInitialise()
	{
	}

	void ImageCopy::doRecordInto( VkCommandBuffer commandBuffer )const
	{
		// Put source image in transfer source layout.
		m_graph.memoryBarrier( commandBuffer
			, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			, VK_PIPELINE_STAGE_TRANSFER_BIT
			, m_srcAttach
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		// Put target image in transfer destination layout.
		m_graph.memoryBarrier( commandBuffer
			, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			, VK_PIPELINE_STAGE_TRANSFER_BIT
			, m_dstAttach
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		// Copy source to target.
		VkImageCopy copyRegion{ convert( m_srcAttach.view.data->info.subresourceRange )
			, {}
			, convert( m_dstAttach.view.data->info.subresourceRange )
			, {}
			, m_copySize };
		m_context.vkCmdCopyImage( commandBuffer
			, m_srcImage
			, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, m_dstImage
			, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			, 1u
			, &copyRegion );
	}

	VkPipelineStageFlags ImageCopy::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
}