/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"

#include <vector>

namespace crg
{
	template< typename DataT >
	struct DataTransitionT
	{
		DataT data;
		Attachment outputAttach;
		Attachment inputAttach;
	};

	bool operator==( ViewTransition const & lhs, ViewTransition const & rhs );
	bool operator==( BufferTransition const & lhs, BufferTransition const & rhs );

	struct AttachmentTransitions
	{
		ViewTransitionArray viewTransitions;
		BufferTransitionArray bufferTransitions;
	};

	AttachmentTransitions mergeIdenticalTransitions( AttachmentTransitions value );

	struct FramePassTransitions
	{
		FramePass const * pass;
		AttachmentTransitions transitions;
	};
}
