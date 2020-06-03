/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg
{
	namespace builder
	{
		GraphNodePtrArray buildGraph( RenderPassPtrArray const & passes
			, RootNode & rootNode
			, AttachmentTransitionArray & allAttaches
			, RenderPassDependenciesArray const & dependencies );
	}
}
