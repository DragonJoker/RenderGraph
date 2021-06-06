/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/GenerateMipmaps.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace
	{
		template< typename T >
		inline constexpr T getSubresourceDimension( T const & extent
			, uint32_t mipLevel )noexcept
		{
			return std::max( T( 1 ), T( extent >> mipLevel ) );
		}
	}

	GenerateMipmaps::GenerateMipmaps( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph
		, VkImageLayout outputLayout
		, uint32_t maxPassCount
		, uint32_t const * passIndex )
		: RunnablePass{ pass
			, context
			, graph
			, maxPassCount }
		, m_outputLayout{ outputLayout }
		, m_passIndex{ passIndex }
	{
	}

	void GenerateMipmaps::doInitialise()
	{
		auto & attach = m_pass.images.front();

		for ( auto passIndex = 0u; passIndex < m_commandBuffers.size(); ++passIndex )
		{
			auto viewId = attach.view( passIndex );
			doUpdateFinalLayout( viewId
				, ( m_outputLayout != VK_IMAGE_LAYOUT_UNDEFINED
					? m_outputLayout
					: m_graph.getOutputLayout( m_pass, viewId ) ) );
		}
	}

	void GenerateMipmaps::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto viewId{ m_pass.images.front().view( index ) };
		auto imageId{ viewId.data->image };
		auto image{ m_graph.getImage( imageId ) };
		auto transition = doGetTransition( viewId );
		auto extent = getExtent( imageId );
		auto format = getFormat( imageId );
		auto srcImageLayout = transition.neededLayout;
		auto dstImageLayout = transition.toLayout;

		auto const width = int32_t( extent.width );
		auto const height = int32_t( extent.height );
		auto const depth = int32_t( extent.depth );
		auto const aspectMask = getAspectMask( format );
		auto baseArrayLayer = viewId.data->info.subresourceRange.baseArrayLayer;
		auto layerCount = viewId.data->info.subresourceRange.layerCount;
		auto mipLevels = imageId.data->info.mipLevels;

		for ( uint32_t i = 0u; i < layerCount; ++i )
		{
			auto layer = baseArrayLayer + i;
			VkImageSubresourceRange mipSubRange{ aspectMask
				, 0u
				, 1u
				, layer
				, 1u };
			VkImageBlit imageBlit{};
			imageBlit.dstSubresource.aspectMask = aspectMask;
			imageBlit.dstSubresource.baseArrayLayer = mipSubRange.baseArrayLayer;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = mipSubRange.baseMipLevel;
			imageBlit.dstOffsets[0].x = 0;
			imageBlit.dstOffsets[0].y = 0;
			imageBlit.dstOffsets[0].z = 0;
			imageBlit.dstOffsets[1].x = getSubresourceDimension( width, mipSubRange.baseMipLevel );
			imageBlit.dstOffsets[1].y = getSubresourceDimension( height, mipSubRange.baseMipLevel );
			imageBlit.dstOffsets[1].z = getSubresourceDimension( depth, mipSubRange.baseMipLevel );

			// Transition first mip level to transfer source for read in next iteration
			m_graph.memoryBarrier( commandBuffer
				, imageId
				, mipSubRange
				, transition.fromLayout
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );

			// Copy down mips
			while ( ++mipSubRange.baseMipLevel < mipLevels )
			{
				// Blit from previous level
				// Blit source is previous blit destination
				imageBlit.srcSubresource = imageBlit.dstSubresource;
				imageBlit.srcOffsets[0] = imageBlit.dstOffsets[0];
				imageBlit.srcOffsets[1] = imageBlit.dstOffsets[1];

				// Update blit destination
				imageBlit.dstSubresource.mipLevel = mipSubRange.baseMipLevel;
				imageBlit.dstOffsets[1].x = getSubresourceDimension( width, mipSubRange.baseMipLevel );
				imageBlit.dstOffsets[1].y = getSubresourceDimension( height, mipSubRange.baseMipLevel );
				imageBlit.dstOffsets[1].z = getSubresourceDimension( depth, mipSubRange.baseMipLevel );

				// Transition current mip level to transfer dest
				m_graph.memoryBarrier( commandBuffer
					, imageId
					, mipSubRange
					, VK_IMAGE_LAYOUT_UNDEFINED
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

				// Perform blit
				m_context.vkCmdBlitImage( commandBuffer 
					, image
					, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, image
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, 1u
					, &imageBlit
					, VK_FILTER_LINEAR );

				if ( mipSubRange.baseMipLevel > 1u )
				{
					// Transition previous mip level to wanted output layout
					// (Not the first one, which is handled in RunnablePass::doRecordInto)
					m_graph.memoryBarrier( commandBuffer
						, imageId
						, { mipSubRange.aspectMask
							, mipSubRange.baseMipLevel - 1u
							, 1u
							, mipSubRange.baseArrayLayer
							, 1u }
						, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
						, dstImageLayout );
				}

				if ( mipSubRange.baseMipLevel == ( mipLevels - 1u ) )
				{
					// Transition final mip level to wanted output layout
					m_graph.memoryBarrier( commandBuffer
						, imageId
						, mipSubRange
						, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
						, dstImageLayout );
				}
				else
				{
					// Transition current mip level to transfer source for read in next iteration
					m_graph.memoryBarrier( commandBuffer
						, imageId
						, mipSubRange
						, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
						, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
				}
			}
		}
	}

	VkPipelineStageFlags GenerateMipmaps::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	uint32_t GenerateMipmaps::doGetPassIndex()const
	{
		return m_passIndex
			? *m_passIndex
			: 0u;
	}
}
