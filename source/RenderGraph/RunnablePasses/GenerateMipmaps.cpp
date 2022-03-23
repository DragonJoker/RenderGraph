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
	namespace genMips
	{
		template< typename T >
		inline constexpr T getSubresourceDimension( T const & extent
			, uint32_t mipLevel )noexcept
		{
			return std::max( T( 1 ), T( extent >> mipLevel ) );
		}
	}

	GenerateMipmaps::GenerateMipmaps( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, VkImageLayout outputLayout
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
		, m_outputLayout{ outputLayout
			, getAccessMask( outputLayout )
			, getStageMask( outputLayout ) }
	{
	}

	void GenerateMipmaps::doInitialise()
	{
		auto & attach = m_pass.images.front();

		for ( auto passIndex = 0u; passIndex < m_commandBuffers.size(); ++passIndex )
		{
			auto viewId = attach.view( passIndex );
			auto layoutState = ( m_outputLayout.layout != VK_IMAGE_LAYOUT_UNDEFINED
				? m_outputLayout
				: m_graph.getOutputLayout( m_pass, viewId, false ) );
			doUpdateFinalLayout( passIndex
				, viewId
				, layoutState.layout
				, layoutState.access
				, layoutState.pipelineStage );
		}
	}

	void GenerateMipmaps::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto viewId{ m_pass.images.front().view( index ) };
		auto imageId{ viewId.data->image };
		auto image{ m_graph.createImage( imageId ) };
		auto transition = getTransition( index, viewId );
		auto extent = getExtent( imageId );
		auto format = getFormat( imageId );
		auto baseArrayLayer = viewId.data->info.subresourceRange.baseArrayLayer;
		auto layerCount = viewId.data->info.subresourceRange.layerCount;
		auto mipLevels = imageId.data->info.mipLevels;
		auto srcImageLayout = transition.needed;
		auto dstMipImageLayout = ( viewId.data->info.subresourceRange.levelCount == mipLevels )
			? srcImageLayout
			: transition.to;

		auto const width = int32_t( extent.width );
		auto const height = int32_t( extent.height );
		auto const depth = int32_t( extent.depth );
		auto const aspectMask = getAspectMask( format );

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
			imageBlit.dstOffsets[1].x = genMips::getSubresourceDimension( width, mipSubRange.baseMipLevel );
			imageBlit.dstOffsets[1].y = genMips::getSubresourceDimension( height, mipSubRange.baseMipLevel );
			imageBlit.dstOffsets[1].z = genMips::getSubresourceDimension( depth, mipSubRange.baseMipLevel );

			// Transition first mip level to transfer source for read in next iteration
			m_graph.memoryBarrier( context
				, commandBuffer
				, imageId
				, mipSubRange
				, srcImageLayout.layout
				, { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, VK_ACCESS_TRANSFER_READ_BIT
					, VK_PIPELINE_STAGE_TRANSFER_BIT } );

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
				imageBlit.dstOffsets[1].x = genMips::getSubresourceDimension( width, mipSubRange.baseMipLevel );
				imageBlit.dstOffsets[1].y = genMips::getSubresourceDimension( height, mipSubRange.baseMipLevel );
				imageBlit.dstOffsets[1].z = genMips::getSubresourceDimension( depth, mipSubRange.baseMipLevel );

				// Transition current mip level to transfer dest
				m_graph.memoryBarrier( context
					, commandBuffer
					, imageId
					, mipSubRange
					, VK_IMAGE_LAYOUT_UNDEFINED
					, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
						, getAccessMask( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
						, getStageMask( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) } );

				// Perform blit
				m_context.vkCmdBlitImage( commandBuffer 
					, image
					, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, image
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, 1u
					, &imageBlit
					, VK_FILTER_LINEAR );

				// Transition previous mip level to wanted output layout
				m_graph.memoryBarrier( context
					, commandBuffer
					, imageId
					, { mipSubRange.aspectMask
						, mipSubRange.baseMipLevel - 1u
						, 1u
						, mipSubRange.baseArrayLayer
						, 1u }
					, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, dstMipImageLayout );

				if ( mipSubRange.baseMipLevel == ( mipLevels - 1u ) )
				{
					// Transition final mip level to wanted output layout
					m_graph.memoryBarrier( context
						, commandBuffer
						, imageId
						, mipSubRange
						, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
						, dstMipImageLayout );
				}
				else
				{
					// Transition current mip level to transfer source for read in next iteration
					m_graph.memoryBarrier( context
						,commandBuffer
						, imageId
						, mipSubRange
						, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
						, { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
							, getAccessMask( VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
							, getStageMask( VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ) } );
				}
			}
		}
	}

	VkPipelineStageFlags GenerateMipmaps::doGetSemaphoreWaitFlags()const
	{
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
}
