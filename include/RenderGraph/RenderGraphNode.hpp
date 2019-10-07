/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/RenderPass.hpp"
#include "RenderGraph/RenderPassDependencies.hpp"

namespace crg
{
	struct RenderGraphNode
	{
		RenderPass const * pass;
		RenderPassDependenciesArray attachesToPrev;
		std::vector< RenderGraphNode * > next;
	};
}
