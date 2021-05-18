/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		GraphNodePtrArray buildGraph( FramePassPtrArray const & passes
			, RootNode & rootNode
			, AttachmentTransitionArray & allAttaches
			, FramePassDependenciesArray const & dependencies );
	}
}
