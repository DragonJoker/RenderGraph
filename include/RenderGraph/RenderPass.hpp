/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"

#include <RenderPass/RenderPass.hpp>
#include <RenderPass/RenderPassCreateInfo.hpp>

namespace crg
{
	struct RenderPass
	{
		RenderPass( std::string const & name
			, AttachmentArray const & inputs
			, AttachmentArray const & colourOutputs
			, std::optional< Attachment > const & depthStencilOutput = std::nullopt );

		std::string const name;
		AttachmentArray const inputs;
		AttachmentArray const colourOutputs;
		std::optional< Attachment > const depthStencilOutput;
	};
}
