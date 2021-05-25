/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"

#include <vector>

namespace crg
{
	/**
	*\brief
	*	The transition between two states of an image view.
	*/
	struct AttachmentTransition
	{
		ImageViewId view;
		Attachment outputAttach;
		Attachment inputAttach;
	};
	bool operator==( AttachmentTransition const & lhs, AttachmentTransition const & rhs );

	AttachmentTransitionArray mergeIdenticalTransitions( AttachmentTransitionArray value );
	AttachmentTransitionArray reduceDirectPaths( AttachmentTransitionArray value );
}
