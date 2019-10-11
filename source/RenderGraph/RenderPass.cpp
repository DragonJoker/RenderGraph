/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderPass.hpp"

#include "RenderGraph/Attachment.hpp"

namespace crg
{
	RenderPass::RenderPass( std::string const & name
		, AttachmentArray const & sampled
		, AttachmentArray const & colourInOuts
		, std::optional< Attachment > const & depthStencilInOut )
		: name{ name }
		, sampled{ sampled }
		, colourInOuts{ colourInOuts }
		, depthStencilInOut{ depthStencilInOut }
	{
	}
}
