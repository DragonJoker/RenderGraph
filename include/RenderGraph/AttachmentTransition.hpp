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
	struct ViewTransition
	{
		ImageViewId view;
		Attachment outputAttach;
		Attachment inputAttach;
	};
	bool operator==( ViewTransition const & lhs, ViewTransition const & rhs );
	/**
	*\brief
	*	The transition between two states of a storage buffer.
	*/
	struct BufferTransition
	{
		Buffer buffer;
		Attachment outputAttach;
		Attachment inputAttach;
	};
	bool operator==( BufferTransition const & lhs, BufferTransition const & rhs );

	struct AttachmentTransitions
	{
		ViewTransitionArray viewTransitions;
		BufferTransitionArray bufferTransitions;
	};

	AttachmentTransitions mergeIdenticalTransitions( AttachmentTransitions value );
	AttachmentTransitions reduceDirectPaths( AttachmentTransitions value );
}
