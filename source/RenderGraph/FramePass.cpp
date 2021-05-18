/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/RunnablePass.hpp"

#include <array>

namespace crg
{
	FramePass::FramePass( std::string const & name
		, RunnablePassCreator runnableCreator )
		: FramePass{ name
			, {}
			, {}
			, std::nullopt
			, runnableCreator }
	{
	}
	
	FramePass::FramePass( std::string const & name
		, AttachmentArray const & sampled
		, AttachmentArray const & colourInOuts
		, RunnablePassCreator runnableCreator )
		: FramePass{ name
			, sampled
			, colourInOuts
			, std::nullopt
			, runnableCreator }
	{
	}

	FramePass::FramePass( std::string const & name
		, AttachmentArray const & sampled
		, AttachmentArray const & colourInOuts
		, std::optional< Attachment > const & depthStencilInOut
		, RunnablePassCreator runnableCreator )
		: name{ name }
		, sampled{ sampled }
		, colourInOuts{ colourInOuts }
		, depthStencilInOut{ depthStencilInOut }
		, runnableCreator{ runnableCreator }
	{
	}

	Attachment FramePass::createSampled( ImageViewData viewData
		, VkImageLayout initialLayout
		, VkFilter filter )
	{
		auto result = Attachment::createSampled( viewData
			, initialLayout
			, filter );
		sampled.push_back( result );
		return result;
	}

	Attachment FramePass::createColour( ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		auto result = Attachment::createColour( viewData
			, loadOp
			, storeOp
			, initialLayout
			, finalLayout
			, std::move( clearValue )
			, std::move( blendState ) );
		colourInOuts.push_back( result );
		return result;
	}

	Attachment FramePass::createDepthStencil( ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		auto result = Attachment::createDepthStencil( viewData
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, finalLayout
			, std::move( clearValue ) );
		depthStencilInOut = result;
		return result;
	}

	RunnablePassPtr FramePass::createRunnable( GraphContext const & context
		, RunnableGraph & graph )const
	{
		auto result = runnableCreator( *this, context, graph );
		result->initialise();
		return result;
	}
}
