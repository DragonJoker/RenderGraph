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

	private:
		friend bool operator==( DataTransitionT const & lhs, DataTransitionT const & rhs )
		{
			return match( lhs.data, rhs.data )
				&& lhs.outputAttach == rhs.outputAttach
				&& lhs.inputAttach == rhs.inputAttach;
		}
	};

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
