/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"

namespace crg
{
	struct RenderPassDependencies
	{
		RenderPass const * srcPass;
		RenderPass const * dstPass;
		AttachmentArray srcOutputs;
		AttachmentArray dstInputs;
	};

	inline bool operator==( RenderPassDependencies const & lhs
		, RenderPassDependencies const & rhs )
	{
		return lhs.dstPass == rhs.dstPass
			&& lhs.srcPass == rhs.srcPass
			&& lhs.srcOutputs == rhs.srcOutputs
			&& lhs.dstInputs == rhs.dstInputs;
	}
}
