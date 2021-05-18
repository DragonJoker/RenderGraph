/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"

namespace crg
{
	struct FramePassDependencies
	{
		FramePass const * srcPass{};
		FramePass const * dstPass{};
		AttachmentArray srcOutputs{};
		AttachmentArray dstInputs{};
	};

	inline bool operator==( FramePassDependencies const & lhs
		, FramePassDependencies const & rhs )
	{
		return lhs.dstPass == rhs.dstPass
			&& lhs.srcPass == rhs.srcPass
			&& lhs.srcOutputs == rhs.srcOutputs
			&& lhs.dstInputs == rhs.dstInputs;
	}
}
