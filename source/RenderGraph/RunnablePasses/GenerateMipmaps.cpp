/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePasses/GenerateMipmaps.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <array>

namespace crg
{
	namespace genMips
	{
		template< typename T >
		static constexpr T getSubresourceDimension( T const & extent
			, uint32_t mipLevel )noexcept
		{
			return std::max( T( 1 ), T( extent >> mipLevel ) );
		}
	}

	GenerateMipmaps::GenerateMipmaps( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, ImageLayout outputLayout
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
		, m_outputLayout{ makeLayoutState( outputLayout ) }
	{
	}

	void GenerateMipmaps::doRecordInto( RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )const
	{
		for ( auto const & [_, view] : getPass().getInouts() )
			doProcessImageView( context, commandBuffer, view->view( index ) );
	}

	void GenerateMipmaps::doProcessImageView( RecordContext & context
		, VkCommandBuffer commandBuffer
		, ImageViewId viewId )const
	{
		auto imageId{ viewId.data->image };
		auto image{ getGraph().createImage( imageId ) };
		auto extent = getExtent( imageId );
		auto format = getFormat( imageId );
		auto baseArrayLayer = getSubresourceRange( viewId ).baseArrayLayer;
		auto layerCount = getSubresourceRange( viewId ).layerCount;
		auto mipLevels = imageId.data->info.mipLevels;
		auto nextLayoutState = getGraph().getNextLayoutState( context
			, *this
			, viewId );

		auto const width = int32_t( extent.width );
		auto const height = int32_t( extent.height );
		auto const depth = int32_t( extent.depth );
		auto const aspectMask = getAspectMask( format );

		for ( uint32_t i = 0u; i < layerCount; ++i )
		{
			auto layer = baseArrayLayer + i;
			ImageSubresourceRange mipSubRange{ aspectMask
				, 0u
				, 1u
				, layer
				, 1u };
			VkImageBlit imageBlit{};
			imageBlit.dstSubresource.aspectMask = getImageAspectFlags( aspectMask );
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
			auto firstLayoutState = getGraph().getCurrentLayoutState( context
				, imageId
				, viewId.data->info.viewType
				, mipSubRange );
			context.memoryBarrier( commandBuffer
				, imageId
				, mipSubRange
				, firstLayoutState.layout
				, makeLayoutState( ImageLayout::eTransferSrc ) );

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
				context.memoryBarrier( commandBuffer
					, imageId
					, mipSubRange
					, makeLayoutState( ImageLayout::eTransferDst ) );

				// Perform blit
				context->vkCmdBlitImage( commandBuffer 
					, image
					, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
					, image
					, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					, 1u
					, &imageBlit
					, VK_FILTER_LINEAR );

				// Transition previous mip level to wanted output layout
				context.memoryBarrier( commandBuffer
					, imageId
					, { mipSubRange.aspectMask
						, mipSubRange.baseMipLevel - 1u
						, 1u
						, mipSubRange.baseArrayLayer
						, 1u }
					, ImageLayout::eTransferSrc
					, nextLayoutState );

				if ( mipSubRange.baseMipLevel == ( mipLevels - 1u ) )
				{
					// Transition final mip level to wanted output layout
					context.memoryBarrier( commandBuffer
						, imageId
						, mipSubRange
						, ImageLayout::eTransferDst
						, nextLayoutState );
				}
				else
				{
					// Transition current mip level to transfer source for read in next iteration
					context.memoryBarrier( commandBuffer
						, imageId
						, mipSubRange
						, ImageLayout::eTransferDst
						, makeLayoutState( ImageLayout::eTransferSrc ) );
				}
			}
		}
	}
}
