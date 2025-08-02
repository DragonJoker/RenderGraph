/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

namespace crg::builder
{
	AttachmentArray findEndPoints( FramePassArray const & passes );
	void buildGraph( AttachmentArray const & endPoints
		, RootNode & root
		, GraphNodePtrArray & graph
		, bool separateDepthStencilLayouts );
}
