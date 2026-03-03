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

	GraphNode::~GraphNode()noexcept = default;

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
		, FramePassGroup const & passGroup )
		: kind{ pkind }
		, id{ pid }
		, name{ std::move( pname ) }
		, group{ &passGroup }
	{
	}

	void GraphNode::attachNode( GraphNode & child )
	{
		if ( prev.end() == std::find( prev.begin(), prev.end(), &child ) )
			prev.push_back( &child );
	}

	//*********************************************************************************************

	FramePassNode::~FramePassNode()noexcept = default;

	FramePassNode::FramePassNode( FramePass const & framePass )
		: GraphNode{ MyKind, framePass.getId(), framePass.getGroupName(), framePass.getGroup() }
		, pass{ &framePass }
	{
	}

	void FramePassNode::accept( GraphVisitor * vis )const
	{
		vis->visitFramePassNode( this );
	}

	//*********************************************************************************************

	RootNode::~RootNode()noexcept = default;

	RootNode::RootNode( FrameGraph const & frameGraph )
		: GraphNode{ MyKind, 0u, frameGraph.getName(), frameGraph.getDefaultGroup() }
		, graph{ &frameGraph }
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
