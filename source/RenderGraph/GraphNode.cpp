/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/GraphNode.hpp"

#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/RenderPass.hpp"

namespace crg
{
	//*********************************************************************************************

	GraphNode::~GraphNode()
	{
	}

	GraphNode::GraphNode( Kind kind
		, std::string name
		, AttachmentsNodeMap attachments )
		: kind{ kind }
		, name{ std::move( name ) }
		, next{}
		, attachsToPrev{ std::move( attachments ) }
	{
	}

	void GraphNode::addAttaches( GraphAdjacentNode prev, AttachmentTransitionArray attachsToPrev )
	{
		bool dirty = false;
		auto * mine = &this->attachsToPrev[prev];

		for ( auto & attach : attachsToPrev )
		{
			auto it = std::find( mine->begin()
				, mine->end()
				, attach );

			if ( it == mine->end() )
			{
				mine->push_back( std::move( attach ) );
				dirty = true;
			}
		}

		if ( dirty )
		{
			*mine = mergeIdenticalTransitions( std::move( *mine ) );
		}
	}

	void GraphNode::attachNode( GraphAdjacentNode next, AttachmentTransitionArray attachsToNext )
	{
		auto it = std::find( this->next.begin()
			, this->next.end()
			, next );

		if ( it == this->next.end() )
		{
			this->next.push_back( next );
		}

		next->addAttaches( this, std::move( attachsToNext ) );
	}

	GraphAdjacentNode GraphNode::findInNext( RenderPass const & pass )const
	{
		auto it = std::find_if( next.begin()
			, next.end()
			, [&pass]( GraphAdjacentNode lookup )
			{
				return getRenderPass( *lookup ) == &pass;
			} );
		return ( next.end() != it )
			? *it
			: nullptr;
	}

	AttachmentTransitionArray const & GraphNode::getAttachsToPrev( ConstGraphAdjacentNode const pred )const
	{
		auto it = attachsToPrev.find( pred );
		assert( it != attachsToPrev.end() );
		return it->second;
	}

	//*********************************************************************************************

	RenderPassNode::RenderPassNode( RenderPass const & pass )
		: GraphNode{ MyKind, pass.name, {} }
		, pass{ &pass }
	{
	}

	void RenderPassNode::accept( GraphVisitor * vis )
	{
		vis->visitRenderPassNode( this );
	}

	//*********************************************************************************************

	RootNode::RootNode( std::string name )
		: GraphNode{ MyKind, std::move( name ), {} }
	{
	}

	void RootNode::accept( GraphVisitor * vis )
	{
		vis->visitRootNode( this );
	}

	//*********************************************************************************************

	RenderPass const * getRenderPass( GraphNode const & node )
	{
		if ( !isRenderPassNode( node ) )
		{
			return nullptr;
		}

		return &nodeCast< RenderPassNode >( node ).getRenderPass();
	}

	//*********************************************************************************************
}
