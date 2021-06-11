/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		void buildPassAttachDependencies( GraphNodePtrArray const & nodes
			, FramePassDependencies & inputTransitions
			, FramePassDependencies & outputTransitions
			, AttachmentTransitions & allTransitions );
	}
}
