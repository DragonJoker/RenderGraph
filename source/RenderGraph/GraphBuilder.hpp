/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		void buildGraph( RootNode & rootNode
			, GraphNodePtrArray const & passes
			, PassDependencyCache & imgDepsCache
			, PassDependencyCache & bufDepsCache
			, AttachmentTransitions & transitions );
	}
}
