/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/GraphNode.hpp"

#include "RenderGraph/GraphVisitor.hpp"

namespace crg
{
	//*********************************************************************************************

	GraphNode::~GraphNode()
	{
	}

	GraphNode::GraphNode( Kind kind )
		: m_kind{ kind }
	{
	}

	//*********************************************************************************************

	RenderPassNode::RenderPassNode( RenderPass const * pass
		, RenderPassDependenciesArray attachesToPrev )
		: GraphNode{ kind }
		, pass{ std::move( pass ) }
		, attachesToPrev{ std::move( attachesToPrev ) }
	{
	}

	void RenderPassNode::accept( GraphVisitor * vis )
	{
		vis->visitRenderPassNode( this );
	}

	//*********************************************************************************************

	RootNode::RootNode()
		: GraphNode{ kind }
	{
	}

	void RootNode::accept( GraphVisitor * vis )
	{
		vis->visitRootNode( this );
	}

	//*********************************************************************************************

	PhiNode::PhiNode( std::vector< GraphNode * > nodes )
		: GraphNode{ kind }
		, nodes{ std::move( nodes ) }
	{
	}

	void PhiNode::accept( GraphVisitor * vis )
	{
		vis->visitPhiNode( this );
	}

	//*********************************************************************************************
}
