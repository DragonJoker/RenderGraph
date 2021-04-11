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

	RunnablePassPtr RenderPass::createRunnable( GraphContext const & context
		, RunnableGraph const & graph )const
	{
		auto result = runnableCreator( *this, context, graph );
		result->initialise();
		return result;
	}
}
