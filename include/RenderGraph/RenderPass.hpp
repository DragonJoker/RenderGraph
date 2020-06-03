/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"

#include <optional>

namespace crg
{
	struct RenderPass
	{
		RenderPass( std::string const & name
			, AttachmentArray const & sampled
			, AttachmentArray const & colourInOuts
			, std::optional< Attachment > const & depthStencilInOut = std::nullopt );

		std::string name;
		AttachmentArray sampled;
		AttachmentArray colourInOuts;
		std::optional< Attachment > depthStencilInOut;
	};
}
