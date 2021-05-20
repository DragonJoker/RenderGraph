/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		GraphNodePtrArray buildGraph( RootNode & rootNode
			, FramePassDependenciesMap const & dependencies
			, AttachmentTransitionArray const & transitions );
	}
}
