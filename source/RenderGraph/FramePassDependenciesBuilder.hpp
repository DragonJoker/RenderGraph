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
		FramePassDependenciesMap buildPassAttachDependencies( FramePassPtrArray const & passes
			, AttachmentTransitionArray & allTransitions );
	}
}
