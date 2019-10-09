/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RenderPassDependencies.hpp"

#include <cassert>
#include <map>

namespace crg
{
	/**
	*\brief
	*	Base class for all graph nodes
	*\remarks
	*	It holds a name, the next nodes, and its dependencies from the previous nodes.
	*/
	struct GraphNode
	{
		enum class Kind
		{
			Undefined,
			Root,
			RenderPass,
		};

		virtual ~GraphNode();
		void attachNode( GraphAdjacentNode node, AttachmentArray attaches );
		GraphAdjacentNode findInNext( RenderPass const & pass )const;
		virtual void accept( GraphVisitor * vis ) = 0;
		virtual AttachmentArray const & getAttachsToPrev( ConstGraphAdjacentNode pred = nullptr )const;

		template< typename NodeT >
		NodeT const & cast()const
		{
			assert( kind == NodeT::MyKind );
			return static_cast< NodeT const & >( *this );
		}

		template< typename NodeT >
		NodeT & cast()
		{
			assert( kind == NodeT::MyKind );
			return static_cast< NodeT & >( *this );
		}

		inline Kind getKind()const
		{
			return kind;
		}

		inline std::string const & getName()const
		{
			return name;
		}

		inline auto & getNext()const
		{
			return next;
		}

	protected:
		GraphNode( Kind kind
			, std::string name
			, AttachmentsNodeMap attachments );

	protected:
		Kind kind;
		std::string name;
		GraphAdjacentNodeArray next;
		AttachmentsNodeMap attachsToPrev;
	};
	/**
	*\brief
	*	Graph node coming from a render pass
	*\remarks
	*	The node name wil be the RenderPass name.
	*/
	struct RenderPassNode
		: public GraphNode
	{
		static constexpr Kind MyKind = Kind::RenderPass;

		RenderPassNode( RenderPass const & pass );
		void accept( GraphVisitor * vis )override;

		inline RenderPass const & getRenderPass()const
		{
			return *pass;
		}

	private:
		RenderPass const * pass;
	};
	/**
	*\brief
	*	Root node for the graph.
	*\remarks
	*	Logically, it has no dependency with a previous node.
	*/
	struct RootNode
		: public GraphNode
	{
		static constexpr Kind MyKind = Kind::Root;

		RootNode( std::string name );
		void accept( GraphVisitor * vis )override;
	};

	RenderPass const * getRenderPass( GraphNode const & node );

	inline bool isRenderPassNode( GraphNode const & node )
	{
		return node.getKind() == RenderPassNode::MyKind;
	}

	inline bool isRootNode( GraphNode const & node )
	{
		return node.getKind() == RootNode::MyKind;
	}

	inline bool isRenderPassNode( ConstGraphAdjacentNode node )
	{
		return node && isRenderPassNode( *node );
	}

	inline bool isRootNode( ConstGraphAdjacentNode node )
	{
		return node && isRootNode( *node );
	}

	inline bool isRenderPassNode( GraphNodePtr const & node )
	{
		return node && isRenderPassNode( *node );
	}

	inline bool isRootNode( GraphNodePtr const & node )
	{
		return node && isRootNode( *node );
	}

	template< typename NodeT >
	NodeT const & nodeCast( GraphNode const & node )
	{
		return node.cast< NodeT >();
	}

	template< typename NodeT >
	NodeT & nodeCast( GraphNode & node )
	{
		return node.cast< NodeT >();
	}
}
