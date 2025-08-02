/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/GraphNode.hpp"

#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/FrameGraph.hpp"
#include "RenderGraph/FramePass.hpp"

namespace crg
{
	//*********************************************************************************************

	GraphNode::GraphNode( GraphNode && rhs )noexcept
		: kind{ rhs.kind }
		, id{ rhs.id }
		, name{ std::move( rhs.name ) }
		, group{ rhs.group }
		, prev{ std::move( rhs.prev ) }
	{
		rhs.kind = Kind::Undefined;
		rhs.id = 0u;
	}

	GraphNode::GraphNode( Kind pkind
		, uint32_t pid
		, std::string pname
		, FramePassGroup const & pgroup )
		: kind{ pkind }
		, id{ pid }
		, name{ std::move( pname ) }
		, group{ pgroup }
	{
	}

	void GraphNode::attachNode( GraphNode & child )
	{
		if ( prev.end() == std::find( prev.begin(), prev.end(), &child ) )
			prev.push_back( &child );
	}

	//*********************************************************************************************

	FramePassNode::FramePassNode( FramePass const & pass )
		: GraphNode{ MyKind, pass.id, pass.getGroupName(), pass.group }
		, pass{ &pass }
	{
	}

	void FramePassNode::accept( GraphVisitor * vis )const
	{
		vis->visitFramePassNode( this );
	}

	//*********************************************************************************************

	RootNode::RootNode( FrameGraph const & pgraph )
		: GraphNode{ MyKind, 0u, pgraph.getName(), pgraph.getDefaultGroup() }
		, graph{ &pgraph }
	{
	}

	void RootNode::accept( GraphVisitor * vis )const
	{
		vis->visitRootNode( this );
	}

	//*********************************************************************************************

	FramePass const * getFramePass( GraphNode const & node )
	{
		if ( !isFramePassNode( node ) )
		{
			return nullptr;
		}

		return &nodeCast< FramePassNode >( node ).getFramePass();
	}

	//*********************************************************************************************
}
