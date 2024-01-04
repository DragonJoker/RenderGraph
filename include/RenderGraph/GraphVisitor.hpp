/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/GraphNode.hpp"

namespace crg
{
	class GraphVisitor
	{
	public:
		CRG_API virtual ~GraphVisitor()noexcept = default;
		CRG_API virtual void visitRootNode( RootNode const * node ) = 0;
		CRG_API virtual void visitFramePassNode( FramePassNode const * node ) = 0;
	};
}
