/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"

#include <vector>

namespace crg
{
	/**
	*\brief
	*	The attachment, and the passes related to it, all as an input or all as anoutput.
	*/
	struct AttachmentPasses
	{
		Attachment attachment;
		std::set< RenderPass const * > passes;
	};
	using AttachmentPassesArray = std::vector< AttachmentPasses >;
	bool operator==( AttachmentPasses const & lhs, AttachmentPasses const & rhs );
	/**
	*\brief
	*	The transition between two states of an attachment.
	*/
	struct AttachmentTransition
	{
		// Passes from this will expect the attachment to be an output.
		AttachmentPassesArray srcOutputs;
		// Passes from this will expect the attachment to be an input.
		AttachmentPasses dstInput;
	};
	bool operator==( AttachmentTransition const & lhs, AttachmentTransition const & rhs );

	AttachmentTransitionArray mergeIdenticalTransitions( AttachmentTransitionArray value );
	AttachmentTransitionArray mergeTransitionsPerInput( AttachmentTransitionArray value );
	AttachmentTransitionArray reduceDirectPaths( AttachmentTransitionArray value );
}
