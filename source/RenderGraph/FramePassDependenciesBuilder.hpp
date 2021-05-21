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
		void buildPassAttachDependencies( FramePassPtrArray const & passes
			, FramePassDependenciesMap & inputTransitions
			, FramePassDependenciesMap & outputTransitions
			, AttachmentTransitionArray & allTransitions );
	}
}
