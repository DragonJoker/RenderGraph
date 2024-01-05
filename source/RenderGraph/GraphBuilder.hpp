/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg::builder
{
	void buildGraph( RootNode & rootNode
		, GraphNodePtrArray const & passes
		, PassDependencyCache & imgDepsCache
		, PassDependencyCache & bufDepsCache
		, AttachmentTransitions const & transitions );
}
