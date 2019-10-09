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

	void GraphNode::attachNode( GraphAdjacentNode node, AttachmentArray attaches )
	{
		auto it = std::find( next.begin()
			, next.end()
			, node );
		if ( it == next.end() )
		{
			next.push_back( node );
			node->attachsToPrev[this] = std::move( attaches );
		}
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

	AttachmentArray const & GraphNode::getAttachsToPrev( ConstGraphAdjacentNode const pred )const
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
