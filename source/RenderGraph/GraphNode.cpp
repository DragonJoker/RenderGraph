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

	GraphNode::GraphNode( Kind kind
		, uint32_t id
		, std::string name )
		: kind{ kind }
		, id{ id }
		, name{ std::move( name ) }
		, next{}
	{
	}

	void GraphNode::addAttaches( GraphAdjacentNode prev
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
		auto it = std::find( next.begin()
			, next.end()
			, nextNode );

		if ( it == next.end() )
		{
			this->next.push_back( nextNode );
		}

		nextNode->addAttaches( this
			, std::move( nextInputAttaches ) );
	}

	GraphAdjacentNode GraphNode::findInNext( FramePass const & pass )const
	{
		auto it = std::find_if( next.begin()
			, next.end()
			, [&pass]( GraphAdjacentNode lookup )
			{
				return getFramePass( *lookup ) == &pass;
			} );
		return ( next.end() != it )
			? *it
			: nullptr;
	}

	bool GraphNode::hasInNext( ConstGraphAdjacentNode const & node )const
	{
		auto it = std::find_if( next.begin()
			, next.end()
			, [&node]( GraphAdjacentNode lookup )
			{
				return lookup == node;
			} );
		return it != next.end();
	}

	AttachmentTransitions const & GraphNode::getInputAttaches( ConstGraphAdjacentNode const pred )const
	{
		auto it = inputAttaches.find( pred );
		assert( it != inputAttaches.end() );
		return it->second;
	}

	//*********************************************************************************************

	FramePassNode::FramePassNode( FramePass const & pass )
		: GraphNode{ MyKind, pass.id, pass.name }
		, pass{ &pass }
	{
	}

	void FramePassNode::accept( GraphVisitor * vis )const
	{
		vis->visitFramePassNode( this );
	}

	//*********************************************************************************************

	RootNode::RootNode( FrameGraph const & pgraph )
		: GraphNode{ MyKind, 0u, pgraph.getName() }
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
