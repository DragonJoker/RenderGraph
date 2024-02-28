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
		, next{ std::move( rhs.next ) }
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

	void GraphNode::addAttaches( ConstGraphAdjacentNode prev
		, AttachmentTransitions inAttaches )
	{
		bool dirty = false;
		auto * mine = &this->inputAttaches[prev];

		for ( auto & attach : inAttaches.viewTransitions )
		{
			auto it = std::find( mine->viewTransitions.begin()
				, mine->viewTransitions.end()
				, attach );

			if ( it == mine->viewTransitions.end() )
			{
				mine->viewTransitions.push_back( std::move( attach ) );
				dirty = true;
			}
		}

		for ( auto & attach : inAttaches.bufferTransitions )
		{
			auto it = std::find( mine->bufferTransitions.begin()
				, mine->bufferTransitions.end()
				, attach );

			if ( it == mine->bufferTransitions.end() )
			{
				mine->bufferTransitions.push_back( std::move( attach ) );
				dirty = true;
			}
		}

		if ( dirty )
		{
			*mine = mergeIdenticalTransitions( std::move( *mine ) );
		}
	}

	void GraphNode::attachNode( GraphAdjacentNode nextNode
		, AttachmentTransitions nextInputAttaches )
	{
		if ( auto it = std::find( next.begin()
			, next.end()
			, nextNode ); it == next.end() )
		{
			this->next.push_back( nextNode );
		}

		nextNode->addAttaches( this
			, std::move( nextInputAttaches ) );
	}

	AttachmentTransitions const & GraphNode::getInputAttaches( ConstGraphAdjacentNode const pred )const
	{
		auto it = inputAttaches.find( pred );
		assert( it != inputAttaches.end() );
		return it->second;
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
