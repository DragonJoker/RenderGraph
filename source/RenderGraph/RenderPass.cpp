/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderPass.hpp"
#include "RenderGraph/RunnablePass.hpp"

#include <array>

namespace crg
{
	RenderPass::RenderPass( std::string const & name
		, RunnablePassCreator runnableCreator )
		: RenderPass{ name
			, {}
			, {}
			, std::nullopt
			, runnableCreator }
	{
	}
	
	RenderPass::RenderPass( std::string const & name
		, AttachmentArray const & sampled
		, AttachmentArray const & colourInOuts
		, RunnablePassCreator runnableCreator )
		: RenderPass{ name
			, sampled
			, colourInOuts
			, std::nullopt
			, runnableCreator }
	{
	}

	RenderPass::RenderPass( std::string const & name
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

	Attachment RenderPass::createSampled( ImageViewData viewData
		, VkImageLayout initialLayout
		, VkFilter filter )
	{
		auto result = Attachment::createSampled( viewData
			, initialLayout
			, filter );
		sampled.push_back( result );
		return result;
	}

	Attachment RenderPass::createColour( ImageViewData viewData
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

	Attachment RenderPass::createDepthStencil( ImageViewData viewData
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

	RunnablePassPtr RenderPass::createRunnable( GraphContext const & context
		, RunnableGraph & graph )const
	{
		auto result = runnableCreator( *this, context, graph );
		result->initialise();
		return result;
	}
}
