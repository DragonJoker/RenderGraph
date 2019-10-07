/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/GraphNode.hpp"

namespace crg
{
	class GraphVisitor
	{
	public:
		virtual ~GraphVisitor() = default;
		virtual void visitRootNode( RootNode * node ) = 0;
		virtual void visitRenderPassNode( RenderPassNode * node ) = 0;
		virtual void visitPhiNode( PhiNode * node ) = 0;
	};
}
